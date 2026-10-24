require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroPdf"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported, :visionos => 1.0 }
  s.source       = { :git => "https://github.com/nitinshukla413/react-native-nitro-html-to-pdf.git", :tag => "#{s.version}" }

  s.source_files = [
    "ios/**/*.{m,mm}",
    "cpp/*.{hpp,cpp}",
    "cpp/core/*.{hpp,cpp}",
  ]
  s.preserve_paths = [
    "cpp/third_party/litehtml/include/**/*.h",
    "cpp/third_party/litehtml/src/**/*.h",
    "cpp/third_party/stb/stb_image.h",
  ]

  litehtml = "$(PODS_TARGET_SRCROOT)/cpp/third_party/litehtml"
  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => [
      "\"#{litehtml}/include\"",
      "\"$(PODS_ROOT)/Headers/Public/NitroModules\"",
      "\"$(PODS_ROOT)/Headers/Private/NitroModules\"",
    ].join(" "),
    # Header maps ignore case across pods: litehtml's "iterators.h" would resolve to folly's "Iterators.h".
    "USE_HEADERMAP" => "NO",
  }
  s.frameworks = "CoreText"
  s.libraries = "z"

  # litehtml and gumbo need their own include directories.
  s.subspec "litehtml" do |ss|
    ss.source_files = [
      "cpp/third_party/litehtml/src/*.cpp",
      "cpp/third_party/litehtml/src/gumbo/*.c",
    ]
    ss.compiler_flags = [
      "-w",
      "-iquote \"#{litehtml}/src\"",
      "-iquote \"#{litehtml}/include/litehtml\"",
      "-iquote \"#{litehtml}/src/gumbo/include\"",
      "-iquote \"#{litehtml}/src/gumbo/include/gumbo\"",
    ].join(" ")
  end

  load 'nitrogen/generated/ios/NitroPdf+autolinking.rb'
  add_nitrogen_files(s)

  s.dependency 'React-jsi'
  s.dependency 'React-callinvoker'
  install_modules_dependencies(s)
end
