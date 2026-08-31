import SwiftUI

/// The on-screen pad exactly as it will look over a game, shown where there is no game.
///
/// WHY IT REUSES THE REAL PAD RATHER THAN DRAWING ITS OWN
///
/// A preview that redraws the layout separately is a second description of where every
/// control is, and the two drift the moment one is edited - which is precisely how the
/// pad ended up not matching the console in the first place. This is the same
/// OptimizedControlPanel the game gets, given callbacks that go nowhere and hit testing
/// switched off, so it cannot disagree with the real thing about position, size, skin,
/// which controls are hidden, or transparency.
///
/// The transparency in particular is the point of the preview. The pad is drawn at the
/// user's opacity setting over live gameplay, and a solid mock-up tells you nothing about
/// whether you will still be able to see the game underneath it. So this renders at the
/// same opacity, over a background standing in for the frame behind it.
struct ControllerPreview: View {
    let skin: WiiUControllerSkin
    /// A stand-in for the game behind the pad. Something with structure rather than a
    /// flat colour, because judging translucency against a plain field is judging it
    /// against the easiest possible case.
    var backdrop: LinearGradient = LinearGradient(
        colors: [Color(red: 0.10, green: 0.11, blue: 0.16),
                 Color(red: 0.22, green: 0.16, blue: 0.24),
                 Color(red: 0.08, green: 0.10, blue: 0.14)],
        startPoint: .topLeading,
        endPoint: .bottomTrailing)

    /// Never editable here. Edit mode is a live interaction on a running game - it turns
    /// the buttons off and adds drag handles - and offering it in a preview would let
    /// somebody drag a layout while looking at a picture of one.
    @State private var neverEditing = false

    var body: some View {
        ZStack {
            backdrop
            // A few shapes so the translucency has something to be translucent OVER.
            // Without them the preview would answer "can you see through it" with a
            // colour rather than with a picture.
            GeometryReader { geo in
                let side = min(geo.size.width, geo.size.height)
                Circle()
                    .fill(Color.white.opacity(0.06))
                    .frame(width: side * 0.55, height: side * 0.55)
                    .offset(x: geo.size.width * 0.28, y: -side * 0.10)
                RoundedRectangle(cornerRadius: side * 0.06)
                    .fill(Color.white.opacity(0.05))
                    .frame(width: side * 0.42, height: side * 0.30)
                    .offset(x: geo.size.width * 0.06, y: side * 0.34)
            }

            OptimizedControlPanel(
                skin: skin,
                onInput: { _, _ in },
                onStick: { _, _ in },
                isEditingLayout: $neverEditing
            )
            // Belt and braces. The callbacks above already go nowhere, but a preview that
            // silently accepts presses would let somebody hold a button down in Settings
            // and then wonder why the next game starts with it held.
            .allowsHitTesting(false)
        }
        .clipShape(RoundedRectangle(cornerRadius: 14, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 14, style: .continuous)
                .strokeBorder(Color.white.opacity(0.10), lineWidth: 1)
        )
        // 16:9, because the pad sizes itself from the container's SHORT side and a preview
        // with the wrong proportions would size its controls differently from the game.
        .aspectRatio(16.0 / 9.0, contentMode: .fit)
    }
}
