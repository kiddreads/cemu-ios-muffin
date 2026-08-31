import Foundation

/// Everything the library knows about a game that cannot be derived from the file itself.
///
/// WHY THIS EXISTS
///
/// There was no persistence of any kind. `GameMetadata` declared a `CodingKeys` enum that
/// deliberately omitted `isFavorite`, and the `games.json` filename was declared once and
/// never referenced anywhere in the app - so the library was rebuilt from a directory scan
/// on every launch and favourites were silently discarded every time the app restarted.
///
/// Annotations rather than a replacement library. The ROMs on disk stay the source of
/// truth for what exists; this only carries what the filesystem cannot say. Losing this
/// file costs a favourite and a badge, never a game, which is why it can be thrown away
/// on any doubt rather than repaired.
struct GameAnnotation: Codable {
    /// Matches `GameMetadata.id`, which is the filename stem.
    var id: String
    var isFavorite: Bool = false
    /// Filled in later by a title probe. Optional because the probe is not free and has
    /// not necessarily run yet - and "not looked at" must stay distinguishable from
    /// "looked at and found nothing".
    var titleName: String?
    var titleId: String?
    var lastPlayed: Date?
}

private struct LibraryFile: Codable {
    var version: Int
    var games: [GameAnnotation]
}

/// Loads and saves annotations. Deliberately dumb and unfailable: every error path ends in
/// "carry on with nothing", because a library that refuses to open is far worse than one
/// that has forgotten which games were favourites.
@MainActor
final class GameLibraryStore {
    /// Bump when the shape changes incompatibly. A file from the future or the past is
    /// ignored rather than migrated - there is nothing here worth the risk of a migration
    /// bug, and it regenerates by itself.
    private static let currentVersion = 1

    /// In Documents, NOT inside Roms/. That directory is scanned for games, and a stray
    /// json in it is one more thing for the scanner to have an opinion about. The dead
    /// `games.json` constant pointed there, which is part of why it was never used.
    private static var fileURL: URL? {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
            .first?.appendingPathComponent("library.json")
    }

    private var annotations: [String: GameAnnotation] = [:]
    private var saveTask: Task<Void, Never>?

    func load() {
        guard let url = Self.fileURL,
              let data = try? Data(contentsOf: url),
              let file = try? JSONDecoder().decode(LibraryFile.self, from: data),
              file.version == Self.currentVersion
        else { return }
        annotations = Dictionary(uniqueKeysWithValues: file.games.map { ($0.id, $0) })
    }

    func annotation(for id: String) -> GameAnnotation? { annotations[id] }

    func update(_ annotation: GameAnnotation) {
        annotations[annotation.id] = annotation
        scheduleSave()
    }

    /// Drops annotations whose game is no longer on disk, so deleting a ROM does not leave
    /// its favourite behind to reattach itself to a different game that happens to be
    /// imported under the same name later.
    func prune(toKeep ids: Set<String>) {
        let before = annotations.count
        annotations = annotations.filter { ids.contains($0.key) }
        if annotations.count != before {
            scheduleSave()
        }
    }

    /// Debounced. Toggling a favourite should not write the file on every tap, and the
    /// half-second is short enough that nothing realistic loses a change.
    private func scheduleSave() {
        saveTask?.cancel()
        saveTask = Task { [weak self] in
            try? await Task.sleep(nanoseconds: 500_000_000)
            guard !Task.isCancelled else { return }
            self?.saveNow()
        }
    }

    func saveNow() {
        guard let url = Self.fileURL else { return }
        let file = LibraryFile(version: Self.currentVersion,
                               games: annotations.values.sorted { $0.id < $1.id })
        guard let data = try? JSONEncoder().encode(file) else { return }
        // Atomic, so a kill mid-write leaves the previous file rather than a truncated one
        // that would then fail to decode and take every favourite with it.
        try? data.write(to: url, options: .atomic)
    }
}
