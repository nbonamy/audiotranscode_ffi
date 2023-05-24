
Pod::Spec.new do |s|
  s.name             = 'audiotranscode_ffi'
  s.version          = '1.1.0'
  s.summary          = 'Audio Transcoding using FFmpeg'
  s.description      = 'Audio Transcoding using FFmpeg for Dart/Flutter'
  s.homepage         = 'https://github.com/nbonamy/audiotranscode_ffi'
  s.license          = { :file => '../LICENSE', :type => 'MIT' }
  s.author           = { 'Nicolas Bonamy' => 'nbonamy@gmail.com' }

  s.source           = { :git => 'https://github.com/nbonamy/audiotranscode_ffi.git', :tag => s.version.to_s }
  s.source_files     = 'Classes/**/*'
  s.dependency 'FlutterMacOS'

  s.dependency 'ffmpeg-kit-macos-audio', '5.1'
  s.dependency 'FLAC-framework', '1.4.2-2'

  s.platform = :osx, '13.0'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'

end
