#include "Cafe/HW/Espresso/Const.h"
#include "config/ActiveSettings.h"
#include "util/helpers/fspinlock.h"
#include "util/highresolutiontimer/HighResolutionTimer.h"
#include "Common/cpu_features.h"
#include "Cemu/Logging/CemuLogging.h"

#include <atomic>
#include <vector>

#if defined(ARCH_X86_64)
#include <immintrin.h>
#pragma intrinsic(__rdtsc)
#endif

uint64 _rdtscLastMeasure = 0;
uint64 _rdtscFrequency = 0;

struct uint128_t
{
	uint64 low;
	uint64 high;
};

static_assert(sizeof(uint128_t) == 16);

uint128_t _rdtscAcc{};

uint64 muldiv64(uint64 a, uint64 b, uint64 d)
{
	uint64 diva = a / d;
	uint64 moda = a % d;
	uint64 divb = b / d;
	uint64 modb = b % d;
	return diva * b + moda * divb + moda * modb / d;
}

#if defined(__aarch64__)
// Defined further down, next to the rest of the lock-free timebase; declared here
// because PPCTimer_init() and PPCTimer_start() above it both publish an anchor.
static void PPCTimer_republishAnchor(bool resetToZero);
#endif

uint64 PPCTimer_estimateRDTSCFrequency()
{
    #if defined(ARCH_X86_64)
	if (!g_CPUFeatures.x86.invariant_tsc)
		cemuLog_log(LogType::Force, "Invariant TSC not supported");
    #endif

	_mm_mfence();
	uint64 tscStart = __rdtsc();
	unsigned int startTime = GetTickCount();
	HRTick startTick = HighResolutionTimer::now().getTick();
	// wait roughly 3 seconds
	while (true)
	{
		if ((GetTickCount() - startTime) >= 3000)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	_mm_mfence();
	HRTick stopTick = HighResolutionTimer::now().getTick();
	uint64 tscEnd = __rdtsc();
	// derive frequency approximation from measured time difference
	uint64 tsc_diff = tscEnd - tscStart;
	uint64 hrtFreq = 0;
	uint64 hrtDiff = HighResolutionTimer::getTimeDiffEx(startTick, stopTick, hrtFreq);
	uint64 tsc_freq = muldiv64(tsc_diff, hrtFreq, hrtDiff);

	// uint64 freqMultiplier = tsc_freq / hrtFreq;
	//cemuLog_log(LogType::Force, "RDTSC measurement test:");
	//cemuLog_log(LogType::Force, "TSC-diff:   0x{:016x}", tsc_diff);
	//cemuLog_log(LogType::Force, "TSC-freq:   0x{:016x}", tsc_freq);
	//cemuLog_log(LogType::Force, "HPC-diff:   0x{:016x}", qpc_diff);
	//cemuLog_log(LogType::Force, "HPC-freq:   0x{:016x}", (uint64)qpc_freq.QuadPart);
	//cemuLog_log(LogType::Force, "Multiplier: 0x{:016x}", freqMultiplier);

	return tsc_freq;
}

int PPCTimer_initThread()
{
	_rdtscFrequency = PPCTimer_estimateRDTSCFrequency();
	return 0;
}

void PPCTimer_init()
{
#if defined(__aarch64__)
	// cntfrq_el0 IS the counter frequency, exactly, so there is nothing to estimate.
	// That also removes the detached thread that spent three seconds measuring it and
	// the PPCTimer_waitForInit() poll that waited on the result - three seconds of boot,
	// and a window during which PPCTimer_isReady() reported false.
	uint64 f;
	asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
	_rdtscFrequency = f;
	_rdtscLastMeasure = __rdtsc();
	PPCTimer_republishAnchor(true);
	cemuLog_log(LogType::Force, "Emulated timebase: counter at {} Hz, read lock-free", f);
#else
	std::thread t(PPCTimer_initThread);
	t.detach();
	_rdtscLastMeasure = __rdtsc();
#endif
}

uint64 _tickSummary = 0;

void PPCTimer_start()
{
	_rdtscLastMeasure = __rdtsc();
	_tickSummary = 0;
#if defined(__aarch64__)
	// Rebase to zero for the new title, which is what _tickSummary = 0 does above.
	PPCTimer_republishAnchor(true);
#endif
}

uint64 PPCTimer_getRawTsc()
{
	return __rdtsc();
}

uint64 PPCTimer_microsecondsToTsc(uint64 us)
{
	return (us * _rdtscFrequency) / 1000000ULL;
}

uint64 PPCTimer_tscToMicroseconds(uint64 us)
{
	uint128_t r{};
	r.low = _umul128(us, 1000000ULL, &r.high);

	uint64 remainder;
	const uint64 microseconds = _udiv128(r.high, r.low, _rdtscFrequency, &remainder);

	return microseconds;
}

bool PPCTimer_isReady()
{
	return _rdtscFrequency != 0;
}

void PPCTimer_waitForInit()
{
	while (!PPCTimer_isReady()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

FSpinlock sTimerSpinlock;

#if defined(__aarch64__)

// The guest timebase, lock-free.
//
// WHAT WAS WRONG
//
// Every read took a global spinlock, issued a full memory fence, did a 128-bit multiply
// into a shared accumulator, and then divided that accumulator by the counter frequency.
// On arm64 that division is _udiv128, which has no instruction - it lowers to a call to
// __udivti3, a software divide - and the spinlock's backoff is _mm_pause, which is
// `isb sy`, a full instruction-synchronisation barrier.
//
// The contention is the worse half. This is reached from every guest mftb, from
// OSGetSystemTime and OSGetTime and their siblings, from __OSLoadThread once per
// timeslice per core, from coreinit's spinlocks and message queues, from GX2 - and from
// the Latte GPU thread. So three interpreter threads and the GPU thread serialise on one
// lock to run a software division, several hundred cycles at a time, on a path that a
// title polling the clock hits constantly.
//
// WHY IT CAN SIMPLY BE DELETED HERE
//
// The accumulator exists for x86 reasons. There, __rdtsc is not architecturally
// invariant and the frequency is ESTIMATED at runtime over three seconds, so the
// remainder has to be carried to stop the error compounding, and monotonicity has to be
// enforced by hand. On arm64 the counter is cntvct_el0: architecturally monotonic,
// uniform across cores, and its exact frequency is readable from cntfrq_el0. Nothing has
// to be estimated, so nothing has to be corrected.
//
// The tick therefore becomes a pure function of the counter and an immutable anchor,
// which is what makes it lock-free: readers only ever read.
//
// THE DIVISION BECOMES A MULTIPLY
//
// tick = counterDelta * CORE_CLOCK / cntfrq. With cntfrq known and fixed, the reciprocal
// is precomputed once as a 64.64 fixed-point multiplier and the runtime operation is a
// 128-bit multiply taking the high half - one umulh. Error is under one tick per read
// and, crucially, is computed from the anchor rather than accumulated, so it cannot
// drift no matter how long a title runs.
struct TimebaseAnchor
{
	uint64 cntAtAnchor;   // counter value when this anchor was published
	uint64 tickAtAnchor;  // guest tick at that moment
	uint64 mulFixed;      // CORE_CLOCK / cntfrq, as 64.64 fixed point
	uint8 shift;          // ActiveSettings timer shift in force
};

static std::atomic<TimebaseAnchor*> s_timebaseAnchor{nullptr};
// Retired anchors. A reader may still be holding one when it is replaced, and these are
// 32 bytes and replaced a handful of times in a run (a title start, a speed change), so
// they are kept rather than freed. That is a bounded leak by design, not an oversight -
// reclaiming them safely would mean hazard pointers for no benefit.
static std::vector<TimebaseAnchor*> s_retiredAnchors;
static FSpinlock s_anchorWriteLock;

static uint64 PPCTimer_armCounterFrequency()
{
	uint64 f;
	asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
	return f;
}

static inline uint64 PPCTimer_mulHigh(uint64 a, uint64 b)
{
	return (uint64)(((unsigned __int128)a * (unsigned __int128)b) >> 64);
}

// Publishes a new anchor that continues from wherever the current one had reached, so the
// timebase is continuous across a rebase or a speed change and can never jump or go
// backwards. Writers are rare; readers never block.
static void PPCTimer_republishAnchor(bool resetToZero)
{
	s_anchorWriteLock.lock();

	const uint64 freq = PPCTimer_armCounterFrequency();
	// 64.64 fixed point: floor(CORE_CLOCK * 2^64 / freq).
	const uint64 mulFixed = (uint64)(((unsigned __int128)Espresso::CORE_CLOCK << 64) / (unsigned __int128)freq);
	const uint64 now = __rdtsc();

	uint64 carriedTick = 0;
	if (!resetToZero)
	{
		if (TimebaseAnchor* previous = s_timebaseAnchor.load(std::memory_order_acquire))
		{
			const uint64 delta = now - previous->cntAtAnchor;
			carriedTick = previous->tickAtAnchor + ((PPCTimer_mulHigh(delta, previous->mulFixed) << 3ull) >> previous->shift);
		}
	}

	auto* fresh = new TimebaseAnchor{now, carriedTick, mulFixed, ActiveSettings::GetTimerShiftFactor()};
	TimebaseAnchor* old = s_timebaseAnchor.exchange(fresh, std::memory_order_acq_rel);
	if (old)
		s_retiredAnchors.push_back(old);

	s_anchorWriteLock.unlock();
}

void PPCTimer_onTimerShiftFactorChanged()
{
	PPCTimer_republishAnchor(false);
}

// thread safe, and genuinely lock-free: one acquire load and arithmetic over fields that
// never change after publication.
uint64 PPCTimer_getFromRDTSC()
{
	TimebaseAnchor* anchor = s_timebaseAnchor.load(std::memory_order_acquire);
	if (!anchor) [[unlikely]]
	{
		PPCTimer_republishAnchor(true);
		anchor = s_timebaseAnchor.load(std::memory_order_acquire);
		if (!anchor)
			return 0;
	}
	// cntvct_el0 is monotonic, so this subtraction cannot go negative and needs no clamp.
	const uint64 delta = __rdtsc() - anchor->cntAtAnchor;
	return anchor->tickAtAnchor + ((PPCTimer_mulHigh(delta, anchor->mulFixed) << 3ull) >> anchor->shift);
}

#else

// thread safe
uint64 PPCTimer_getFromRDTSC()
{
	sTimerSpinlock.lock();
	_mm_mfence();
	uint64 rdtscCurrentMeasure = __rdtsc();
	uint64 rdtscDif = rdtscCurrentMeasure - _rdtscLastMeasure;
	// optimized max(rdtscDif, 0) without conditionals
	rdtscDif = rdtscDif & ~(uint64)((sint64)rdtscDif >> 63);

	uint128_t diff{};
	diff.low = _umul128(rdtscDif, Espresso::CORE_CLOCK, &diff.high);

	if(rdtscCurrentMeasure > _rdtscLastMeasure)
		_rdtscLastMeasure = rdtscCurrentMeasure; // only travel forward in time

	uint8 c = 0;
	#if BOOST_OS_WINDOWS
	c = _addcarry_u64(c, _rdtscAcc.low, diff.low, &_rdtscAcc.low);
	_addcarry_u64(c, _rdtscAcc.high, diff.high, &_rdtscAcc.high);
	#else
	// requires casting because of long / long long nonesense
	c = _addcarry_u64(c, _rdtscAcc.low, diff.low, (unsigned long long*)&_rdtscAcc.low);
	_addcarry_u64(c, _rdtscAcc.high, diff.high, (unsigned long long*)&_rdtscAcc.high);
	#endif

	uint64 remainder;
	uint64 elapsedTick = _udiv128(_rdtscAcc.high, _rdtscAcc.low, _rdtscFrequency, &remainder);

	_rdtscAcc.low = remainder;
	_rdtscAcc.high = 0;

	// timer scaling
	elapsedTick <<= 3ull; // *8
	uint8 timerShiftFactor = ActiveSettings::GetTimerShiftFactor();
	elapsedTick >>= timerShiftFactor;

	_tickSummary += elapsedTick;

	sTimerSpinlock.unlock();
	return _tickSummary;
}

// The x86 path reads ActiveSettings::GetTimerShiftFactor() on every call, so a change
// takes effect on its own and there is nothing to republish.
void PPCTimer_onTimerShiftFactorChanged()
{
}

#endif // !__aarch64__
