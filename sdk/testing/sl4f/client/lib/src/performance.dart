// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';
import 'dart:io' show File, Platform, Process, ProcessResult;

import 'package:logging/logging.dart';
import 'package:meta/meta.dart';

import 'dump.dart';
import 'sl4f_client.dart';

String _traceExtension({bool binary, bool compress}) {
  String extension = 'json';
  if (binary) {
    extension = 'fxt';
  }
  if (compress) {
    extension += '.gz';
  }
  return extension;
}

String _traceNameToTargetPath(String traceName, String extension) {
  return '/tmp/$traceName-trace.$extension';
}

final _log = Logger('Performance');

class Performance {
  final Sl4f _sl4f;
  final Dump _dump;

  /// Constructs a [Performance] object.
  Performance(this._sl4f, [Dump dump])
      : _dump = dump ?? Dump();

  /// Closes the underlying HTTP client.
  ///
  /// This need not be called if the Sl4f client is closed instead.
  void close() {
    _sl4f.close();
  }

  /// Starts tracing for the given [duration].
  ///
  /// If [binary] is true, then the trace will be captured in Fuchsia Trace
  /// Format (by default, it is in Chrome JSON Format). If [compress] is true,
  /// the trace will be gzip-compressed. The trace output will be saved to a
  /// path implied by [traceName], [binary], and [compress], and can be
  /// retrieved later via [downloadTraceFile].
  Future<bool> trace(
      {@required Duration duration,
      @required String traceName,
      String categories,
      int bufferSize,
      bool binary = false,
      bool compress = false}) {
    // Invoke `/bin/trace record --duration=$duration --categories=$categories
    // --output-file=$outputFile --buffer-size=$bufferSize` on the target
    // device via ssh.
    final durationSeconds = duration.inSeconds;
    String command = 'trace record --duration=$durationSeconds';
    if (categories != null) {
      command += ' --categories=$categories';
    }
    if (bufferSize != null) {
      command += ' --buffer-size=$bufferSize';
    }
    if (binary) {
      command += ' --binary';
    }
    if (compress) {
      command += ' --compress';
    }
    final String extension =
        _traceExtension(binary: binary, compress: compress);
    final outputFile = _traceNameToTargetPath(traceName, extension);
    if (outputFile != null) {
      command += ' --output-file=$outputFile';
    }
    return _sl4f.ssh(command);
  }

  /// Copies the trace file specified by [traceName] off of the target device,
  /// and then saves it to the dump directory.
  ///
  /// A [trace] call with the same [traceName], [binary], and [compress] must
  /// have successfully completed before calling [downloadTraceFile].
  ///
  /// Returns the download trace [File].
  Future<File> downloadTraceFile(String traceName,
      {bool binary = false, bool compress = false}) async {
    _log.info('Performance: Downloading trace $traceName');
    final String extension =
        _traceExtension(binary: binary, compress: compress);
    final tracePath = _traceNameToTargetPath(traceName, extension);
    final String response = await _sl4f
        .request('traceutil_facade.GetTraceFile', {'path': tracePath});
    return _dump.writeAsBytes(
        '$traceName-trace', extension, base64.decode(response));
  }

  /// A helper function that runs a process with the given args.
  /// Required by the test to capture the parameters passed to [Process.run].
  ///
  /// Returns [true] if the process ran successufly, [false] otherwise.
  Future<bool> runTraceProcessor(String processor, List<String> args) async {
    final ProcessResult results = await Process.run(processor, args);
    _log..info(results.stdout)..info(results.stderr);
    return results.exitCode == 0;
  }

  /// Runs the provided processor ([processorPath]) on the given [trace].
  /// It sets the ouptut file location to be the same as the source.
  ///
  /// TODO(PT-179): Use a processor registry instead of passing the [processorPath].
  ///
  /// Returns the benchmark result [File] generated by the processor.
  Future<File> processTrace(String processorPath, File trace, String testName,
      {String appName}) async {
    final outputFileName =
        '${trace.parent.absolute.path}/$testName-benchmark.json';
    List<String> args = [
      '-test_suite_name=$testName',
      if (appName != null) '-flutter_app_name=$appName',
      '-benchmarks_out_filename=$outputFileName',
      trace.absolute.path
    ];
    final processor = Platform.script.resolve(processorPath).toFilePath();
    if (!await runTraceProcessor(processor, args)) {
      _log.warning('Running the trace processor failed.');
      return null;
    }
    return Future.value(File(outputFileName));
  }
}
