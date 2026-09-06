import Foundation

// GENERATED derivation, not hand-tuned art direction - see the comment on
// MuffinThemeDefinition in MuffinThemeStore.swift for exactly how. Each theme's 14
// tokens are computed from that icon's own real icon.png (median-cut quantized top
// swatches, picked by saturation/lightness for a primary+secondary+accent triad, then
// lightened/darkened/desaturated by fixed formulas - the same lighten-for-dark-accents,
// deepen-and-desaturate-for-dark-backgrounds, flip-text-light-dark relationships
// MuffinTheme's own hand-tuned Bakery theme already documents by hand, just applied
// uniformly across all thirty icons instead of judged one at a time. That means some
// of these will read better than others - a from-scratch art pass per icon was out of
// scope here - but every value traces back to a real pixel in that icon's actual art,
// never an invented hex code.
enum MuffinThemePresets {

    /// The app's original, hand-tuned palette (see MuffinTheme.swift's own header
    /// comment) - kept as literal constants rather than re-derived, so switching this
    /// theme system in changes nothing for anyone who never opens the theme picker.
    static let bakery = MuffinThemeDefinition(
        id: "bakery", name: "Bakery (Original)", iconId: "original",
        backgroundTopLight: "#F6A94F", backgroundTopDark: "#7A4A22",
        backgroundBottomLight: "#E5652E", backgroundBottomDark: "#4A2410",
        muffinTopLightLight: "#E3A254", muffinTopLightDark: "#C98A46",
        muffinTopDarkLight: "#A8622A", muffinTopDarkDark: "#8A4E20",
        creamLight: "#FDF6EC", creamDark: "#241813",
        wrapperLight: "#F0DFC3", wrapperDark: "#3A2A1E",
        blueberryNavyLight: "#453765", blueberryNavyDark: "#8177AD",
        pixelBlueLight: "#6C63FF", pixelBlueDark: "#8A82FF",
        blushPinkLight: "#F2A6A0", blushPinkDark: "#E08880",
        brownDarkestLight: "#2E1B10", brownDarkestDark: "#FBEBD8",
        brownDarkLight: "#5C2E10", brownDarkDark: "#E8CBA8",
        brownMidLight: "#7A4A22", brownMidDark: "#C9A47C",
        sparkleCreamLight: "#FFF3DD", sparkleCreamDark: "#FFF3DD",
        shadowLight: "#4A2410", shadowDark: "#000000"
    )

    static let adhdAwareness = MuffinThemeDefinition(
        id: "adhd-awareness", name: "ADHD Awareness", iconId: "adhd-awareness",
        backgroundTopLight: "#F9A064", backgroundTopDark: "#6C4931",
        backgroundBottomLight: "#B0744B", backgroundBottomDark: "#473122",
        muffinTopLightLight: "#E46E1F", muffinTopLightDark: "#E46E1F",
        muffinTopDarkLight: "#A04D16", muffinTopDarkDark: "#C45F1B",
        creamLight: "#FEF6F0", creamDark: "#2B1D14",
        wrapperLight: "#FEE8DA", wrapperDark: "#432D1F",
        blueberryNavyLight: "#693614", blueberryNavyDark: "#A2826D",
        pixelBlueLight: "#FAE0C1", pixelBlueDark: "#FBE4CA",
        blushPinkLight: "#FAEAD7", blushPinkDark: "#FAEAD7",
        brownDarkestLight: "#23160E", brownDarkestDark: "#FFF8F4",
        brownDarkLight: "#4B301E", brownDarkDark: "#FEEFE5",
        brownMidLight: "#865636", brownMidDark: "#FDE1CD",
        sparkleCreamLight: "#FDF6F2", sparkleCreamDark: "#FDF6F2",
        shadowLight: "#3C2618", shadowDark: "#000000"
    )

    static let audhdAwareness = MuffinThemeDefinition(
        id: "audhd-awareness", name: "AuDHD Awareness", iconId: "audhd-awareness",
        backgroundTopLight: "#EB9623", backgroundTopDark: "#644315",
        backgroundBottomLight: "#A66B1D", backgroundBottomDark: "#412C10",
        muffinTopLightLight: "#F8C868", muffinTopLightDark: "#F8C868",
        muffinTopDarkLight: "#AE8C49", muffinTopDarkDark: "#D5AC59",
        creamLight: "#FDF4E9", creamDark: "#281B08",
        wrapperLight: "#FAE6CA", wrapperDark: "#3E290D",
        blueberryNavyLight: "#745F35", blueberryNavyDark: "#A99C82",
        pixelBlueLight: "#FAF3E7", pixelBlueDark: "#FBF5EA",
        blushPinkLight: "#FBF7EF", blushPinkDark: "#FBF7EF",
        brownDarkestLight: "#211505", brownDarkestDark: "#FEF8F0",
        brownDarkLight: "#472D0B", brownDarkDark: "#FCEDDA",
        brownMidLight: "#7F5113", brownMidDark: "#F9DDB9",
        sparkleCreamLight: "#FFFCF6", sparkleCreamDark: "#FFFCF6",
        shadowLight: "#382408", shadowDark: "#000000"
    )

    static let autismAwareness = MuffinThemeDefinition(
        id: "autism-awareness", name: "Autism Muffin", iconId: "autism-awareness",
        backgroundTopLight: "#F3E7AB", backgroundTopDark: "#6C674F",
        backgroundBottomLight: "#AEA67D", backgroundBottomDark: "#474435",
        muffinTopLightLight: "#FFFEFF", muffinTopLightDark: "#FFFEFF",
        muffinTopDarkLight: "#B2B2B2", muffinTopDarkDark: "#DBDADB",
        creamLight: "#FEFDF7", creamDark: "#2B2920",
        wrapperLight: "#FCF9EB", wrapperDark: "#434031",
        blueberryNavyLight: "#7A7A7A", blueberryNavyDark: "#ADADAD",
        pixelBlueLight: "#A13B6A", pixelBlueDark: "#AE567F",
        blushPinkLight: "#BC7D9A", blushPinkDark: "#BC7D9A",
        brownDarkestLight: "#222018", brownDarkestDark: "#FEFDF9",
        brownDarkLight: "#494533", brownDarkDark: "#FDFBF1",
        brownMidLight: "#837D5C", brownMidDark: "#FBF7E4",
        sparkleCreamLight: "#FFFFFF", sparkleCreamDark: "#FFFFFF",
        shadowLight: "#3A3729", shadowDark: "#000000"
    )

    static let bisexualPride = MuffinThemeDefinition(
        id: "bisexual-pride", name: "Bisexual Pride", iconId: "bisexual-pride",
        backgroundTopLight: "#D60270", backgroundTopDark: "#5A0732",
        backgroundBottomLight: "#960551", backgroundBottomDark: "#3B0621",
        muffinTopLightLight: "#0038A8", muffinTopLightDark: "#0038A8",
        muffinTopDarkLight: "#002776", muffinTopDarkDark: "#003090",
        creamLight: "#FBE6F1", creamDark: "#240314",
        wrapperLight: "#F5C2DD", wrapperDark: "#38041F",
        blueberryNavyLight: "#041C4D", blueberryNavyDark: "#637291",
        pixelBlueLight: "#FAF2E5", pixelBlueDark: "#FBF4E9",
        blushPinkLight: "#FBF6EE", blushPinkDark: "#FBF6EE",
        brownDarkestLight: "#1E0010", brownDarkestDark: "#FCEDF5",
        brownDarkLight: "#400122", brownDarkDark: "#F8D4E7",
        brownMidLight: "#74013C", brownMidDark: "#F2AED1",
        sparkleCreamLight: "#F0F3FA", sparkleCreamDark: "#F0F3FA",
        shadowLight: "#33001B", shadowDark: "#000000"
    )

    static let blueberryBlast = MuffinThemeDefinition(
        id: "blueberry-blast", name: "Blueberry Blast", iconId: "blueberry-blast",
        backgroundTopLight: "#5362C8", backgroundTopDark: "#282E57",
        backgroundBottomLight: "#3E488E", backgroundBottomDark: "#1C2039",
        muffinTopLightLight: "#EACDA9", muffinTopLightDark: "#EACDA9",
        muffinTopDarkLight: "#A49076", muffinTopDarkDark: "#C9B091",
        creamLight: "#EEEFFA", creamDark: "#101323",
        wrapperLight: "#D6D9F2", wrapperDark: "#191D36",
        blueberryNavyLight: "#6F6253", blueberryNavyDark: "#A69E94",
        pixelBlueLight: "#314193", pixelBlueDark: "#4E5CA2",
        blushPinkLight: "#7680B2", blushPinkDark: "#7680B2",
        brownDarkestLight: "#0C0E1C", brownDarkestDark: "#F3F4FB",
        brownDarkLight: "#191D3C", brownDarkDark: "#E2E4F6",
        brownMidLight: "#2D356C", brownMidDark: "#C8CDED",
        sparkleCreamLight: "#FEFCFA", sparkleCreamDark: "#FEFCFA",
        shadowLight: "#141830", shadowDark: "#000000"
    )

    static let dark = MuffinThemeDefinition(
        id: "dark", name: "Dark Mode", iconId: "dark",
        backgroundTopLight: "#EFD5B1", backgroundTopDark: "#6A5F51",
        backgroundBottomLight: "#AB9981", backgroundBottomDark: "#464037",
        muffinTopLightLight: "#976842", muffinTopLightDark: "#976842",
        muffinTopDarkLight: "#6A492E", muffinTopDarkDark: "#825939",
        creamLight: "#FDFBF7", creamDark: "#2A2621",
        wrapperLight: "#FBF5EC", wrapperDark: "#423B33",
        blueberryNavyLight: "#473222", blueberryNavyDark: "#8D8076",
        pixelBlueLight: "#1B100C", pixelBlueDark: "#3B312E",
        blushPinkLight: "#635C5A", blushPinkDark: "#635C5A",
        brownDarkestLight: "#211E19", brownDarkestDark: "#FEFCFA",
        brownDarkLight: "#484035", brownDarkDark: "#FCF8F2",
        brownMidLight: "#817360", brownMidDark: "#FAF2E6",
        sparkleCreamLight: "#F9F6F4", sparkleCreamDark: "#F9F6F4",
        shadowLight: "#39332A", shadowDark: "#000000"
    )

    static let disabilityPride = MuffinThemeDefinition(
        id: "disability-pride", name: "Disability Pride", iconId: "disability-pride",
        backgroundTopLight: "#C4833C", backgroundTopDark: "#553B1F",
        backgroundBottomLight: "#8B5E2D", backgroundBottomDark: "#372716",
        muffinTopLightLight: "#5F373B", muffinTopLightDark: "#5F373B",
        muffinTopDarkLight: "#422629", muffinTopDarkDark: "#522F33",
        creamLight: "#F9F3EC", creamDark: "#22180C",
        wrapperLight: "#F1E1D0", wrapperDark: "#352513",
        blueberryNavyLight: "#2D1B1D", blueberryNavyDark: "#7D7273",
        pixelBlueLight: "#DAD2C2", pixelBlueDark: "#DFD8CB",
        blushPinkLight: "#E5E0D6", blushPinkDark: "#E5E0D6",
        brownDarkestLight: "#1B1208", brownDarkestDark: "#FBF6F1",
        brownDarkLight: "#3B2712", brownDarkDark: "#F5EADE",
        brownMidLight: "#6A4720", brownMidDark: "#ECD7C1",
        sparkleCreamLight: "#F5F3F3", sparkleCreamDark: "#F5F3F3",
        shadowLight: "#2F1F0E", shadowDark: "#000000"
    )

    static let doubleChocolate = MuffinThemeDefinition(
        id: "double-chocolate", name: "Double Chocolate", iconId: "double-chocolate",
        backgroundTopLight: "#7A5033", backgroundTopDark: "#352419",
        backgroundBottomLight: "#563A26", backgroundBottomDark: "#231811",
        muffinTopLightLight: "#482D1B", muffinTopLightDark: "#482D1B",
        muffinTopDarkLight: "#321F13", muffinTopDarkDark: "#3E2717",
        creamLight: "#F2EEEB", creamDark: "#150F0A",
        wrapperLight: "#DFD5CE", wrapperDark: "#21170F",
        blueberryNavyLight: "#22160E", blueberryNavyDark: "#766F6A",
        pixelBlueLight: "#BFA287", pixelBlueDark: "#C8AF98",
        blushPinkLight: "#D1C0AF", blushPinkDark: "#D1C0AF",
        brownDarkestLight: "#110B07", brownDarkestDark: "#F6F3F1",
        brownDarkLight: "#25180F", brownDarkDark: "#E8E1DC",
        brownMidLight: "#422B1C", brownMidDark: "#D4C7BE",
        sparkleCreamLight: "#F4F2F1", sparkleCreamDark: "#F4F2F1",
        shadowLight: "#1D130C", shadowDark: "#000000"
    )

    static let equality = MuffinThemeDefinition(
        id: "equality", name: "Equality", iconId: "equality",
        backgroundTopLight: "#4451DC", backgroundTopDark: "#23285F",
        backgroundBottomLight: "#343C9C", backgroundBottomDark: "#191C3E",
        muffinTopLightLight: "#EDCFA5", muffinTopLightDark: "#EDCFA5",
        muffinTopDarkLight: "#A69173", muffinTopDarkDark: "#CCB28E",
        creamLight: "#ECEEFC", creamDark: "#0E1026",
        wrapperLight: "#D2D5F7", wrapperDark: "#16193B",
        blueberryNavyLight: "#706351", blueberryNavyDark: "#A69E93",
        pixelBlueLight: "#2E348F", pixelBlueDark: "#4B509F",
        blushPinkLight: "#7478AF", blushPinkDark: "#7478AF",
        brownDarkestLight: "#0A0B1F", brownDarkestDark: "#F2F3FD",
        brownDarkLight: "#141842", brownDarkDark: "#DFE1F9",
        brownMidLight: "#252C77", brownMidDark: "#C3C7F4",
        sparkleCreamLight: "#FEFCFA", sparkleCreamDark: "#FEFCFA",
        shadowLight: "#101335", shadowDark: "#000000"
    )

    static let fixTheWorld = MuffinThemeDefinition(
        id: "fix-the-world", name: "Fix the World", iconId: "fix-the-world",
        backgroundTopLight: "#FFC9AD", backgroundTopDark: "#705B50",
        backgroundBottomLight: "#B6917E", backgroundBottomDark: "#4A3D36",
        muffinTopLightLight: "#FCA1C5", muffinTopLightDark: "#FCA1C5",
        muffinTopDarkLight: "#B0718A", muffinTopDarkDark: "#D98AA9",
        creamLight: "#FFFAF7", creamDark: "#2D2520",
        wrapperLight: "#FFF2EB", wrapperDark: "#463932",
        blueberryNavyLight: "#77505F", blueberryNavyDark: "#AB929C",
        pixelBlueLight: "#C88EDC", pixelBlueDark: "#D09EE1",
        blushPinkLight: "#D8B5E4", blushPinkDark: "#D8B5E4",
        brownDarkestLight: "#241C18", brownDarkestDark: "#FFFBF9",
        brownDarkLight: "#4D3C34", brownDarkDark: "#FFF6F1",
        brownMidLight: "#8A6D5D", brownMidDark: "#FFEEE5",
        sparkleCreamLight: "#FFF9FC", sparkleCreamDark: "#FFF9FC",
        shadowLight: "#3D302A", shadowDark: "#000000"
    )

    static let galaxySpace = MuffinThemeDefinition(
        id: "galaxy-space", name: "Galaxy Space", iconId: "galaxy-space",
        backgroundTopLight: "#EACDA7", backgroundTopDark: "#675C4D",
        backgroundBottomLight: "#A7947A", backgroundBottomDark: "#443D34",
        muffinTopLightLight: "#291659", muffinTopLightDark: "#291659",
        muffinTopDarkLight: "#1D0F3E", muffinTopDarkDark: "#23134D",
        creamLight: "#FDFAF6", creamDark: "#29251F",
        wrapperLight: "#FAF3EA", wrapperDark: "#403930",
        blueberryNavyLight: "#140C29", blueberryNavyDark: "#6D687A",
        pixelBlueLight: "#0B061C", pixelBlueDark: "#2D293C",
        blushPinkLight: "#5A5664", blushPinkDark: "#5A5664",
        brownDarkestLight: "#211D17", brownDarkestDark: "#FEFCF9",
        brownDarkLight: "#463E32", brownDarkDark: "#FBF6F0",
        brownMidLight: "#7E6F5A", brownMidDark: "#F8EFE3",
        sparkleCreamLight: "#F2F1F5", sparkleCreamDark: "#F2F1F5",
        shadowLight: "#383128", shadowDark: "#000000"
    )

    static let happy = MuffinThemeDefinition(
        id: "happy", name: "Happy", iconId: "happy",
        backgroundTopLight: "#F29BCF", backgroundTopDark: "#6B485D",
        backgroundBottomLight: "#AD7195", backgroundBottomDark: "#46313E",
        muffinTopLightLight: "#CB92ED", muffinTopLightDark: "#CB92ED",
        muffinTopDarkLight: "#8E66A6", muffinTopDarkDark: "#AF7ECC",
        creamLight: "#FEF5FA", creamDark: "#2B1D25",
        wrapperLight: "#FCE7F3", wrapperDark: "#422D3A",
        blueberryNavyLight: "#61486F", blueberryNavyDark: "#9D8EA6",
        pixelBlueLight: "#FCE5DE", pixelBlueDark: "#FCE9E3",
        blushPinkLight: "#FCEEEA", blushPinkDark: "#FCEEEA",
        brownDarkestLight: "#22161D", brownDarkestDark: "#FEF8FC",
        brownDarkLight: "#492F3E", brownDarkDark: "#FDEEF7",
        brownMidLight: "#835470", brownMidDark: "#FBDFF0",
        sparkleCreamLight: "#FCF8FE", sparkleCreamDark: "#FCF8FE",
        shadowLight: "#3A2532", shadowDark: "#000000"
    )

    static let holidayFrost = MuffinThemeDefinition(
        id: "holiday-frost", name: "Holiday Frost", iconId: "holiday-frost",
        backgroundTopLight: "#AAD1EC", backgroundTopDark: "#4E5E68",
        backgroundBottomLight: "#7C96A8", backgroundBottomDark: "#353E45",
        muffinTopLightLight: "#7EACD8", muffinTopLightDark: "#7EACD8",
        muffinTopDarkLight: "#587897", muffinTopDarkDark: "#6C94BA",
        creamLight: "#F6FAFD", creamDark: "#1F252A",
        wrapperLight: "#EBF4FA", wrapperDark: "#313A41",
        blueberryNavyLight: "#3F5365", blueberryNavyDark: "#8894A0",
        pixelBlueLight: "#A47B5C", pixelBlueDark: "#B18D73",
        blushPinkLight: "#BEA593", blushPinkDark: "#BEA593",
        brownDarkestLight: "#181D21", brownDarkestDark: "#F9FCFE",
        brownDarkLight: "#333F47", brownDarkDark: "#F1F7FC",
        brownMidLight: "#5C717F", brownMidDark: "#E4F0F9",
        sparkleCreamLight: "#F7FAFD", sparkleCreamDark: "#F7FAFD",
        shadowLight: "#293239", shadowDark: "#000000"
    )

    static let lemonZest = MuffinThemeDefinition(
        id: "lemon-zest", name: "Lemon Zest", iconId: "lemon-zest",
        backgroundTopLight: "#FEE57D", backgroundTopDark: "#6F653C",
        backgroundBottomLight: "#B5A35C", backgroundBottomDark: "#494329",
        muffinTopLightLight: "#FDF2BE", muffinTopLightDark: "#FDF2BE",
        muffinTopDarkLight: "#B1A985", muffinTopDarkDark: "#DAD0A3",
        creamLight: "#FFFCF2", creamDark: "#2C2818",
        wrapperLight: "#FFF9E0", wrapperDark: "#453F25",
        blueberryNavyLight: "#78735D", blueberryNavyDark: "#ABA89B",
        pixelBlueLight: "#5AD8D4", pixelBlueDark: "#71DDDA",
        blushPinkLight: "#93E0DE", blushPinkDark: "#93E0DE",
        brownDarkestLight: "#242012", brownDarkestDark: "#FFFDF6",
        brownDarkLight: "#4C4526", brownDarkDark: "#FFFBE9",
        brownMidLight: "#897C44", brownMidDark: "#FFF7D5",
        sparkleCreamLight: "#FFFEFB", sparkleCreamDark: "#FFFEFB",
        shadowLight: "#3D371E", shadowDark: "#000000"
    )

    static let lesbianPride = MuffinThemeDefinition(
        id: "lesbian-pride", name: "Lesbian Pride", iconId: "lesbian-pride",
        backgroundTopLight: "#9F065E", backgroundTopDark: "#44072A",
        backgroundBottomLight: "#700743", backgroundBottomDark: "#2C061C",
        muffinTopLightLight: "#F18B70", muffinTopLightDark: "#F18B70",
        muffinTopDarkLight: "#A9614E", muffinTopDarkDark: "#CF7860",
        creamLight: "#F5E6EF", creamDark: "#1B0311",
        wrapperLight: "#E8C3D8", wrapperDark: "#2A041A",
        blueberryNavyLight: "#714539", blueberryNavyDark: "#A78C84",
        pixelBlueLight: "#D53525", pixelBlueDark: "#DB5144",
        blushPinkLight: "#DC7A71", blushPinkDark: "#DC7A71",
        brownDarkestLight: "#16010D", brownDarkestDark: "#F8EEF4",
        brownDarkLight: "#30021C", brownDarkDark: "#EFD5E4",
        brownMidLight: "#560333", brownMidDark: "#E0AFCB",
        sparkleCreamLight: "#FEF8F6", sparkleCreamDark: "#FEF8F6",
        shadowLight: "#260117", shadowDark: "#000000"
    )

    static let mentalHealthPride = MuffinThemeDefinition(
        id: "mental-health-pride", name: "Mental Health Pride", iconId: "mental-health-pride",
        backgroundTopLight: "#AA7E44", backgroundTopDark: "#4A3821",
        backgroundBottomLight: "#785B33", backgroundBottomDark: "#302617",
        muffinTopLightLight: "#7ABF8C", muffinTopLightDark: "#7ABF8C",
        muffinTopDarkLight: "#558662", muffinTopDarkDark: "#69A478",
        creamLight: "#F6F2EC", creamDark: "#1E170D",
        wrapperLight: "#EBE0D2", wrapperDark: "#2E2315",
        blueberryNavyLight: "#3C5A44", blueberryNavyDark: "#86998B",
        pixelBlueLight: "#86B880", pixelBlueDark: "#97C292",
        blushPinkLight: "#AECDAB", blushPinkDark: "#AECDAB",
        brownDarkestLight: "#18120A", brownDarkestDark: "#F9F6F2",
        brownDarkLight: "#332614", brownDarkDark: "#F1E9DF",
        brownMidLight: "#5C4425", brownMidDark: "#E4D6C3",
        sparkleCreamLight: "#F7FBF8", sparkleCreamDark: "#F7FBF8",
        shadowLight: "#291E10", shadowDark: "#000000"
    )

    static let mintMatcha = MuffinThemeDefinition(
        id: "mint-matcha", name: "Mint Matcha", iconId: "mint-matcha",
        backgroundTopLight: "#71C998", backgroundTopDark: "#355845",
        backgroundBottomLight: "#538F6D", backgroundBottomDark: "#243A2E",
        muffinTopLightLight: "#A8E4C6", muffinTopLightDark: "#A8E4C6",
        muffinTopDarkLight: "#76A08B", muffinTopDarkDark: "#90C4AA",
        creamLight: "#F1FAF5", creamDark: "#15231C",
        wrapperLight: "#DDF2E6", wrapperDark: "#21372B",
        blueberryNavyLight: "#526C5F", blueberryNavyDark: "#94A49C",
        pixelBlueLight: "#E7EAC8", pixelBlueDark: "#EAEDD0",
        blushPinkLight: "#EEEFDB", blushPinkDark: "#EEEFDB",
        brownDarkestLight: "#101C15", brownDarkestDark: "#F5FBF8",
        brownDarkLight: "#223C2E", brownDarkDark: "#E7F6ED",
        brownMidLight: "#3D6D52", brownMidDark: "#D2EEDE",
        sparkleCreamLight: "#FAFDFC", sparkleCreamDark: "#FAFDFC",
        shadowLight: "#1B3024", shadowDark: "#000000"
    )

    static let neonCyber = MuffinThemeDefinition(
        id: "neon-cyber", name: "Neon Cyber", iconId: "neon-cyber",
        backgroundTopLight: "#09121E", backgroundTopDark: "#04080D",
        backgroundBottomLight: "#070D15", backgroundBottomDark: "#030508",
        muffinTopLightLight: "#233453", muffinTopLightDark: "#233453",
        muffinTopDarkLight: "#18243A", muffinTopDarkDark: "#1E2D47",
        creamLight: "#E6E7E8", creamDark: "#020305",
        wrapperLight: "#C4C6C9", wrapperDark: "#030508",
        blueberryNavyLight: "#121927", blueberryNavyDark: "#6C7079",
        pixelBlueLight: "#100A1E", pixelBlueDark: "#312C3E",
        blushPinkLight: "#5C5965", blushPinkDark: "#5C5965",
        brownDarkestLight: "#010304", brownDarkestDark: "#EEEEEF",
        brownDarkLight: "#030509", brownDarkDark: "#D5D7D9",
        brownMidLight: "#050A10", brownMidDark: "#B0B3B7",
        sparkleCreamLight: "#F2F3F5", sparkleCreamDark: "#F2F3F5",
        shadowLight: "#020407", shadowDark: "#000000"
    )

    static let nonbinaryPride = MuffinThemeDefinition(
        id: "nonbinary-pride", name: "Nonbinary Pride", iconId: "nonbinary-pride",
        backgroundTopLight: "#FCF434", backgroundTopDark: "#6C691D",
        backgroundBottomLight: "#B2AC29", backgroundBottomDark: "#464415",
        muffinTopLightLight: "#A56BCF", muffinTopLightDark: "#A56BCF",
        muffinTopDarkLight: "#734B91", muffinTopDarkDark: "#8E5CB2",
        creamLight: "#FFFEEB", creamDark: "#2B2A0C",
        wrapperLight: "#FEFCCE", wrapperDark: "#434112",
        blueberryNavyLight: "#4F3661", blueberryNavyDark: "#92829D",
        pixelBlueLight: "#F6E9D5", pixelBlueDark: "#F7ECDB",
        blushPinkLight: "#F8F0E4", blushPinkDark: "#F8F0E4",
        brownDarkestLight: "#232207", brownDarkestDark: "#FFFEF1",
        brownDarkLight: "#4C4910", brownDarkDark: "#FEFDDC",
        brownMidLight: "#88841C", brownMidDark: "#FEFBBE",
        sparkleCreamLight: "#FAF6FC", sparkleCreamDark: "#FAF6FC",
        shadowLight: "#3C3B0C", shadowDark: "#000000"
    )

    static let proDiamondIce = MuffinThemeDefinition(
        id: "pro-diamond-ice", name: "Diamond Ice", iconId: "pro-diamond-ice",
        backgroundTopLight: "#DBEEFB", backgroundTopDark: "#636B70",
        backgroundBottomLight: "#9EABB4", backgroundBottomDark: "#43474A",
        muffinTopLightLight: "#9ACAE8", muffinTopLightDark: "#9ACAE8",
        muffinTopDarkLight: "#6C8DA2", muffinTopDarkDark: "#84AEC8",
        creamLight: "#FBFDFF", creamDark: "#282B2D",
        wrapperLight: "#F6FBFE", wrapperDark: "#3E4346",
        blueberryNavyLight: "#4C606D", blueberryNavyDark: "#909CA4",
        pixelBlueLight: "#D85A91", pixelBlueDark: "#DD71A0",
        blushPinkLight: "#E093B5", blushPinkDark: "#E093B5",
        brownDarkestLight: "#1F2123", brownDarkestDark: "#FCFEFF",
        brownDarkLight: "#42474B", brownDarkDark: "#F9FCFE",
        brownMidLight: "#768188", brownMidDark: "#F3FAFE",
        sparkleCreamLight: "#F9FCFE", sparkleCreamDark: "#F9FCFE",
        shadowLight: "#35393C", shadowDark: "#000000"
    )

    static let proGoldVip = MuffinThemeDefinition(
        id: "pro-gold-vip", name: "Gold VIP", iconId: "pro-gold-vip",
        backgroundTopLight: "#ECB22E", backgroundTopDark: "#654E1A",
        backgroundBottomLight: "#A67F25", backgroundBottomDark: "#423313",
        muffinTopLightLight: "#F7CD61", muffinTopLightDark: "#F7CD61",
        muffinTopDarkLight: "#AD9044", muffinTopDarkDark: "#D4B053",
        creamLight: "#FDF7EA", creamDark: "#291F0A",
        wrapperLight: "#FAEDCD", wrapperDark: "#3F3110",
        blueberryNavyLight: "#736132", blueberryNavyDark: "#A89D80",
        pixelBlueLight: "#A77419", pixelBlueDark: "#B38739",
        blushPinkLight: "#BE9F67", blushPinkDark: "#BE9F67",
        brownDarkestLight: "#211906", brownDarkestDark: "#FEFAF0",
        brownDarkLight: "#47350E", brownDarkDark: "#FCF2DB",
        brownMidLight: "#7F6019", brownMidDark: "#F9E6BC",
        sparkleCreamLight: "#FFFCF6", sparkleCreamDark: "#FFFCF6",
        shadowLight: "#392B0B", shadowDark: "#000000"
    )

    static let proHolographic = MuffinThemeDefinition(
        id: "pro-holographic", name: "Holographic", iconId: "pro-holographic",
        backgroundTopLight: "#A8DCF3", backgroundTopDark: "#4D626B",
        backgroundBottomLight: "#7A9EAE", backgroundBottomDark: "#354147",
        muffinTopLightLight: "#ACEEE8", muffinTopLightDark: "#ACEEE8",
        muffinTopDarkLight: "#78A7A2", muffinTopDarkDark: "#94CDC8",
        creamLight: "#F6FCFE", creamDark: "#1F272B",
        wrapperLight: "#EAF7FC", wrapperDark: "#303D43",
        blueberryNavyLight: "#54716E", blueberryNavyDark: "#95A7A5",
        pixelBlueLight: "#D3AEEB", pixelBlueDark: "#D9B9EE",
        blushPinkLight: "#E0CAEF", blushPinkDark: "#E0CAEF",
        brownDarkestLight: "#181F22", brownDarkestDark: "#F9FDFE",
        brownDarkLight: "#324249", brownDarkDark: "#F0F9FD",
        brownMidLight: "#5B7783", brownMidDark: "#E3F4FB",
        sparkleCreamLight: "#FAFEFE", sparkleCreamDark: "#FAFEFE",
        shadowLight: "#28353A", shadowDark: "#000000"
    )

    static let progressPride = MuffinThemeDefinition(
        id: "progress-pride", name: "Progress Pride", iconId: "progress-pride",
        backgroundTopLight: "#FDD582", backgroundTopDark: "#6F5E3E",
        backgroundBottomLight: "#B49960", backgroundBottomDark: "#493F2A",
        muffinTopLightLight: "#DC7B66", muffinTopLightDark: "#DC7B66",
        muffinTopDarkLight: "#9A5647", muffinTopDarkDark: "#BD6A58",
        creamLight: "#FFFBF2", creamDark: "#2C2619",
        wrapperLight: "#FFF5E1", wrapperDark: "#453B26",
        blueberryNavyLight: "#673D34", blueberryNavyDark: "#A18781",
        pixelBlueLight: "#8F64D2", pixelBlueDark: "#9F7AD8",
        blushPinkLight: "#B499DC", blushPinkDark: "#B499DC",
        brownDarkestLight: "#231E12", brownDarkestDark: "#FFFCF6",
        brownDarkLight: "#4C4027", brownDarkDark: "#FFF8EA",
        brownMidLight: "#897346", brownMidDark: "#FEF2D7",
        sparkleCreamLight: "#FDF7F6", sparkleCreamDark: "#FDF7F6",
        shadowLight: "#3D331F", shadowDark: "#000000"
    )

    static let pumpkinSpice = MuffinThemeDefinition(
        id: "pumpkin-spice", name: "Pumpkin Spice", iconId: "pumpkin-spice",
        backgroundTopLight: "#E8BF93", backgroundTopDark: "#665644",
        backgroundBottomLight: "#A68A6B", backgroundBottomDark: "#43392F",
        muffinTopLightLight: "#C16D38", muffinTopLightDark: "#C16D38",
        muffinTopDarkLight: "#874C27", muffinTopDarkDark: "#A65E30",
        creamLight: "#FDF9F4", creamDark: "#29221B",
        wrapperLight: "#F9F0E5", wrapperDark: "#40352B",
        blueberryNavyLight: "#59351E", blueberryNavyDark: "#988274",
        pixelBlueLight: "#864523", pixelBlueDark: "#975F42",
        blushPinkLight: "#A9826D", blushPinkDark: "#A9826D",
        brownDarkestLight: "#201B15", brownDarkestDark: "#FDFBF7",
        brownDarkLight: "#46392C", brownDarkDark: "#FBF4ED",
        brownMidLight: "#7D674F", brownMidDark: "#F8EBDC",
        sparkleCreamLight: "#FBF6F3", sparkleCreamDark: "#FBF6F3",
        shadowLight: "#382E23", shadowDark: "#000000"
    )

    static let rainbowPride = MuffinThemeDefinition(
        id: "rainbow-pride", name: "Rainbow Pride", iconId: "rainbow-pride",
        backgroundTopLight: "#FFC44B", backgroundTopDark: "#6E5627",
        backgroundBottomLight: "#B58C3A", backgroundBottomDark: "#48391B",
        muffinTopLightLight: "#CBE36A", muffinTopLightDark: "#CBE36A",
        muffinTopDarkLight: "#8E9F4A", muffinTopDarkDark: "#AFC35B",
        creamLight: "#FFF9ED", creamDark: "#2C230F",
        wrapperLight: "#FFF1D4", wrapperDark: "#443618",
        blueberryNavyLight: "#606A36", blueberryNavyDark: "#9CA382",
        pixelBlueLight: "#74E076", pixelBlueDark: "#87E489",
        blushPinkLight: "#A4E7A5", blushPinkDark: "#A4E7A5",
        brownDarkestLight: "#241B0B", brownDarkestDark: "#FFFBF2",
        brownDarkLight: "#4D3B17", brownDarkDark: "#FFF5E0",
        brownMidLight: "#8A6A28", brownMidDark: "#FFECC5",
        sparkleCreamLight: "#FCFDF6", sparkleCreamDark: "#FCFDF6",
        shadowLight: "#3D2F12", shadowDark: "#000000"
    )

    static let retro = MuffinThemeDefinition(
        id: "retro", name: "Retro Console", iconId: "retro",
        backgroundTopLight: "#E3C7A3", backgroundTopDark: "#645A4B",
        backgroundBottomLight: "#A28F77", backgroundBottomDark: "#423B33",
        muffinTopLightLight: "#422869", muffinTopLightDark: "#422869",
        muffinTopDarkLight: "#2E1C4A", muffinTopDarkDark: "#39225A",
        creamLight: "#FCF9F6", creamDark: "#28241E",
        wrapperLight: "#F8F2E9", wrapperDark: "#3E382F",
        blueberryNavyLight: "#201531", blueberryNavyDark: "#756E7F",
        pixelBlueLight: "#936045", pixelBlueDark: "#A2765F",
        blushPinkLight: "#B39483", blushPinkDark: "#B39483",
        brownDarkestLight: "#201C17", brownDarkestDark: "#FDFBF9",
        brownDarkLight: "#443C31", brownDarkDark: "#FAF5EF",
        brownMidLight: "#7B6B58", brownMidDark: "#F6EDE2",
        sparkleCreamLight: "#F4F2F6", sparkleCreamDark: "#F4F2F6",
        shadowLight: "#363027", shadowDark: "#000000"
    )

    static let spookyHalloween = MuffinThemeDefinition(
        id: "spooky-halloween", name: "Spooky Halloween", iconId: "spooky-halloween",
        backgroundTopLight: "#814F37", backgroundTopDark: "#38241B",
        backgroundBottomLight: "#5B3A29", backgroundBottomDark: "#251813",
        muffinTopLightLight: "#351C47", muffinTopLightDark: "#351C47",
        muffinTopDarkLight: "#251432", muffinTopDarkDark: "#2E183D",
        creamLight: "#F2EDEB", creamDark: "#170F0B",
        wrapperLight: "#E1D5CF", wrapperDark: "#231711",
        blueberryNavyLight: "#190E21", blueberryNavyDark: "#706A75",
        pixelBlueLight: "#D2AFA5", pixelBlueDark: "#D8BAB2",
        blushPinkLight: "#DFC9C3", blushPinkDark: "#DFC9C3",
        brownDarkestLight: "#120B08", brownDarkestDark: "#F6F3F1",
        brownDarkLight: "#271811", brownDarkDark: "#EAE1DD",
        brownMidLight: "#462B1E", brownMidDark: "#D7C7BF",
        sparkleCreamLight: "#F3F1F4", sparkleCreamDark: "#F3F1F4",
        shadowLight: "#1F130D", shadowDark: "#000000"
    )

    static let strawberry = MuffinThemeDefinition(
        id: "strawberry", name: "Strawberry", iconId: "strawberry",
        backgroundTopLight: "#FFBFD1", backgroundTopDark: "#71585F",
        backgroundBottomLight: "#B68B97", backgroundBottomDark: "#4B3B40",
        muffinTopLightLight: "#F8E7DB", muffinTopLightDark: "#F8E7DB",
        muffinTopDarkLight: "#AEA299", muffinTopDarkDark: "#D5C7BC",
        creamLight: "#FFF9FA", creamDark: "#2D2326",
        wrapperLight: "#FFF0F4", wrapperDark: "#46373B",
        blueberryNavyLight: "#776F6A", blueberryNavyDark: "#ABA6A3",
        pixelBlueLight: "#69D85A", pixelBlueDark: "#7EDD71",
        blushPinkLight: "#9CE093", blushPinkDark: "#9CE093",
        brownDarkestLight: "#241B1D", brownDarkestDark: "#FFFBFC",
        brownDarkLight: "#4D393F", brownDarkDark: "#FFF4F7",
        brownMidLight: "#8A6771", brownMidDark: "#FFEBF0",
        sparkleCreamLight: "#FFFEFD", sparkleCreamDark: "#FFFEFD",
        shadowLight: "#3D2E32", shadowDark: "#000000"
    )

    static let summerBeach = MuffinThemeDefinition(
        id: "summer-beach", name: "Summer Beach", iconId: "summer-beach",
        backgroundTopLight: "#2998AB", backgroundTopDark: "#164249",
        backgroundBottomLight: "#206C79", backgroundBottomDark: "#102B30",
        muffinTopLightLight: "#4DBDC6", muffinTopLightDark: "#4DBDC6",
        muffinTopDarkLight: "#36848B", muffinTopDarkDark: "#42A3AA",
        creamLight: "#EAF5F7", creamDark: "#091A1D",
        wrapperLight: "#CCE6EB", wrapperDark: "#0E292E",
        blueberryNavyLight: "#28585C", blueberryNavyDark: "#7A979A",
        pixelBlueLight: "#A77F50", pixelBlueDark: "#B39168",
        blushPinkLight: "#C0A88B", blushPinkDark: "#C0A88B",
        brownDarkestLight: "#061518", brownDarkestDark: "#F0F8F9",
        brownDarkLight: "#0C2E33", brownDarkDark: "#DBEDF1",
        brownMidLight: "#16525C", brownMidDark: "#BBDEE4",
        sparkleCreamLight: "#F4FBFC", sparkleCreamDark: "#F4FBFC",
        shadowLight: "#0A2429", shadowDark: "#000000"
    )

    static let transgenderPride = MuffinThemeDefinition(
        id: "transgender-pride", name: "Transgender Pride", iconId: "transgender-pride",
        backgroundTopLight: "#5BCEFA", backgroundTopDark: "#2D5B6C",
        backgroundBottomLight: "#4493B1", backgroundBottomDark: "#203C47",
        muffinTopLightLight: "#F3A9B8", muffinTopLightDark: "#F3A9B8",
        muffinTopDarkLight: "#AA7681", muffinTopDarkDark: "#D1919E",
        creamLight: "#EFFAFE", creamDark: "#12242B",
        wrapperLight: "#D8F3FE", wrapperDark: "#1C3943",
        blueberryNavyLight: "#735359", blueberryNavyDark: "#A89498",
        pixelBlueLight: "#DDE6F1", pixelBlueDark: "#E2EAF3",
        blushPinkLight: "#E9EEF5", blushPinkDark: "#E9EEF5",
        brownDarkestLight: "#0D1D23", brownDarkestDark: "#F4FCFF",
        brownDarkLight: "#1B3E4B", brownDarkDark: "#E3F7FE",
        brownMidLight: "#316F87", brownMidDark: "#CBEFFD",
        sparkleCreamLight: "#FEFAFB", sparkleCreamDark: "#FEFAFB",
        shadowLight: "#16313C", shadowDark: "#000000"
    )

    static let all: [MuffinThemeDefinition] = [
        bakery, adhdAwareness, audhdAwareness, autismAwareness, bisexualPride, blueberryBlast, dark, disabilityPride, doubleChocolate, equality, fixTheWorld, galaxySpace, happy, holidayFrost, lemonZest, lesbianPride, mentalHealthPride, mintMatcha, neonCyber, nonbinaryPride, proDiamondIce, proGoldVip, proHolographic, progressPride, pumpkinSpice, rainbowPride, retro, spookyHalloween, strawberry, summerBeach, transgenderPride
    ]
}
