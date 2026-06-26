// swift-tools-version:5.8
import PackageDescription

let version: String = "0.5.5"
let binaryURL: String =
    "https://github.com/Churikov0112/valhalla-mobile/releases/download/v\(version)/valhalla-wrapper.xcframework.zip"
let binaryChecksum: String = "a9fd74d2b5c1c6fd60339845025b68aa5a2b2f6cdbe91d392c30f3a5a8f6f61a"

let package = Package(
    name: "ValhallaMobile",
    platforms: [
        .iOS("16.4")
    ],
    products: [
        .library(
            name: "Valhalla",
            targets: ["Valhalla"]
        )
    ],
    dependencies: [
        .package(
            url: "https://github.com/Rallista/valhalla-openapi-models-swift.git",
            .upToNextMinor(from: "0.3.0")),
        .package(
            url: "https://github.com/UInt2048/Light-Swift-Untar.git", .upToNextMajor(from: "1.0.4")),
        .package(url: "https://github.com/apple/swift-docc-plugin", .upToNextMajor(from: "1.0.0")),
    ],
    targets: [
        .target(
            name: "Valhalla",
            dependencies: [
                "ValhallaObjc",
                "ValhallaWrapper",
                .product(name: "ValhallaConfigModels", package: "valhalla-openapi-models-swift"),
                .product(name: "ValhallaModels", package: "valhalla-openapi-models-swift"),
                .product(name: "Light-Swift-Untar", package: "Light-Swift-Untar"),
            ],
            path: "apple/Sources/Valhalla",
            resources: [
                .process("SupportData")
            ]
        ),
        .target(
            name: "ValhallaObjc",
            dependencies: ["ValhallaWrapper"],
            path: "apple/Sources/ValhallaObjc",
            linkerSettings: [.linkedLibrary("z")]
        ),
        .binaryTarget(
            name: "ValhallaWrapper",
            url: binaryURL,
            checksum: binaryChecksum
        ),
        .testTarget(
            name: "ValhallaTests",
            dependencies: ["Valhalla"],
            path: "apple/Tests/ValhallaTests",
            resources: [.copy("TestData")]
        ),
    ],
    cLanguageStandard: .gnu17,
    cxxLanguageStandard: .cxx20
)
