// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:fxtest/fxtest.dart';
import 'package:meta/meta.dart';

/// Deconstructed Fuchsia Package Url used to precisely target URL components.
class PackageUrl {
  /// Root chunk of the Package URL.
  ///
  /// ```
  /// fuchsia.com
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String host;

  /// First chunk from the URI.
  ///
  /// ```
  /// pkg-name
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String packageName;

  /// Optional. Second chunk in the URI.
  ///
  /// ```
  /// variant
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String packageVariant;

  /// Optional. Value for "hash" querystring value.
  ///
  /// ```
  /// 1234
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String hash;

  /// Component name with the extension.
  ///
  /// ```
  /// component-name.cmx
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String fullComponentName;

  /// Component name without the extension.
  ///
  /// ```
  /// component-name
  /// ```
  ///
  /// from
  ///
  /// ```
  /// fuchsia-pkg://fuchsia.com/pkg-name/variant?hash=1234#meta/component-name.cmx
  /// ```
  final String componentName;

  PackageUrl({
    @required this.host,
    @required this.packageName,
    @required this.packageVariant,
    @required this.hash,
    @required this.fullComponentName,
    @required this.componentName,
  });

  PackageUrl.none()
      : host = null,
        hash = null,
        packageName = null,
        packageVariant = null,
        fullComponentName = null,
        componentName = null;

  /// Breaks out a canonical Fuchsia URL into its constituent parts.
  ///
  /// Parses something like
  /// `fuchsia-pkg://host/package_name/variant?hash=1234#PATH.cmx` into:
  ///
  /// ```dart
  /// PackageUrl(
  ///   'host': 'host',
  ///   'packageName': 'package_name',
  ///   'packageVariant': 'variant',
  ///   'hash': '1234',
  ///   'fullComponentName': 'PATH.cmx',
  ///   'componentName': 'PATH',
  /// );
  /// ```
  factory PackageUrl.fromString(String packageUrl) {
    Uri parsedUri = Uri.parse(packageUrl);

    if (parsedUri.scheme != 'fuchsia-pkg') {
      throw MalformedFuchsiaUrlException(packageUrl);
    }

    return PackageUrl(
      host: parsedUri.host,
      packageName:
          parsedUri.pathSegments.isNotEmpty ? parsedUri.pathSegments[0] : null,
      packageVariant:
          parsedUri.pathSegments.length > 1 ? parsedUri.pathSegments[1] : null,
      hash: parsedUri.queryParameters['hash'],
      fullComponentName: PackageUrl._removeMetaPrefix(parsedUri.fragment),
      componentName: PackageUrl._removeMetaPrefix(
        PackageUrl._removeExtension(parsedUri.fragment),
      ),
    );
  }

  static String _removeMetaPrefix(String fullComponentName) {
    const token = 'meta/';
    return fullComponentName.startsWith(token)
        ? fullComponentName.substring(token.length)
        : fullComponentName;
  }

  static String _removeExtension(String fullComponentName) {
    // Guard against uninteresting edge cases
    if (fullComponentName == null || !fullComponentName.contains('.')) {
      return fullComponentName ?? '';
    }
    return fullComponentName.substring(0, fullComponentName.lastIndexOf('.'));
  }
}
