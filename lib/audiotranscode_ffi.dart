import 'dart:async';
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:ffi/ffi.dart';

import 'audiotranscode_ffi_bindings_generated.dart';

const String _libName = 'audiotranscode_ffi';

/// The dynamic library in which the symbols for [AudiotranscodeFfiBindings] can be found.
final DynamicLibrary _dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

/// The bindings to the native functions in [_dylib].
final AudiotranscodeFfiBindings _bindings = AudiotranscodeFfiBindings(_dylib);

enum TranscodeFormat { mp3, flac }

Future<bool> transcodeMp3(String src, String dst, int bitrate) async {
  final SendPort helperIsolateSendPort = await _helperIsolateSendPort;
  final int requestId = _nextTranscodeRequestId++;
  final _TranscodeRequest request = _TranscodeRequest(
    requestId,
    src,
    dst,
    TranscodeFormat.mp3,
    bitrate: bitrate,
  );
  final Completer<bool> completer = Completer<bool>();
  _transcodeRequests[requestId] = completer;
  helperIsolateSendPort.send(request);
  return completer.future;
}

Future<bool> transcodeFlac(
  String src,
  String dst,
  int samplerate,
  int bitspersample,
) async {
  final SendPort helperIsolateSendPort = await _helperIsolateSendPort;
  final int requestId = _nextTranscodeRequestId++;
  final _TranscodeRequest request = _TranscodeRequest(
    requestId,
    src,
    dst,
    TranscodeFormat.flac,
    samplerate: samplerate,
    bitspersample: bitspersample,
  );
  final Completer<bool> completer = Completer<bool>();
  _transcodeRequests[requestId] = completer;
  helperIsolateSendPort.send(request);
  return completer.future;
}

/// A request to compute `sum`.
///
/// Typically sent from one isolate to another.
class _TranscodeRequest {
  final int id;
  final String src;
  final String dest;
  final TranscodeFormat format;
  final int samplerate;
  final int bitspersample;
  final int bitrate;

  const _TranscodeRequest(
    this.id,
    this.src,
    this.dest,
    this.format, {
    this.samplerate = 0,
    this.bitspersample = 0,
    this.bitrate = 0,
  });
}

/// A response with the result of `sum`.
///
/// Typically sent from one isolate to another.
class _TranscodeResponse {
  final int id;
  final bool result;
  const _TranscodeResponse(this.id, this.result);
}

/// Counter to identify [_TranscodeRequest]s and [_TranscodeResponse]s.
int _nextTranscodeRequestId = 0;

/// Mapping from [_SumRequest] `id`s to the completers corresponding to the correct future of the pending request.
final Map<int, Completer<bool>> _transcodeRequests = <int, Completer<bool>>{};

/// The SendPort belonging to the helper isolate.
Future<SendPort> _helperIsolateSendPort = () async {
  // The helper isolate is going to send us back a SendPort, which we want to
  // wait for.
  final Completer<SendPort> completer = Completer<SendPort>();

  // Receive port on the main isolate to receive messages from the helper.
  // We receive two types of messages:
  // 1. A port to send messages on.
  // 2. Responses to requests we sent.
  final ReceivePort receivePort = ReceivePort()
    ..listen((dynamic data) {
      if (data is SendPort) {
        // The helper isolate sent us the port on which we can sent it requests.
        completer.complete(data);
        return;
      }
      if (data is _TranscodeResponse) {
        // The helper isolate sent us a response to a request we sent.
        final Completer<bool> completer = _transcodeRequests[data.id]!;
        _transcodeRequests.remove(data.id);
        completer.complete(data.result);
        return;
      }
      throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
    });

  // Start the helper isolate.
  await Isolate.spawn((SendPort sendPort) async {
    final ReceivePort helperReceivePort = ReceivePort()
      ..listen((dynamic data) {
        // On the helper isolate listen to requests and respond to them.
        if (data is _TranscodeRequest) {
          int result = 0;
          if (data.format == TranscodeFormat.mp3) {
            result = _bindings.transcode_to_mp3(
              data.src.toNativeUtf8().cast<Char>(),
              data.dest.toNativeUtf8().cast<Char>(),
              data.bitrate,
            );
          } else if (data.format == TranscodeFormat.flac) {
            result = _bindings.transcode_to_flac(
              data.src.toNativeUtf8().cast<Char>(),
              data.dest.toNativeUtf8().cast<Char>(),
              data.samplerate,
              data.bitspersample,
            );
          }
          final _TranscodeResponse response = _TranscodeResponse(
            data.id,
            result == 1,
          );
          sendPort.send(response);
          return;
        }
        throw UnsupportedError('Unsupported message type: ${data.runtimeType}');
      });

    // Send the the port to the main isolate on which we can receive requests.
    sendPort.send(helperReceivePort.sendPort);
  }, receivePort.sendPort);

  // Wait until the helper isolate has sent us back the SendPort on which we
  // can start sending requests.
  return completer.future;
}();
