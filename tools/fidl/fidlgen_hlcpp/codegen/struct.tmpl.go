// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package codegen

const structTemplate = `
{{- define "StructForwardDeclaration" }}
class {{ .Name }};
{{- end }}

{{- define "StructDeclaration" }}
{{range .DocComments}}
///{{ . }}
{{- end}}
class {{ .Name }} final {
 public:
  static const fidl_type_t* FidlType;

  {{- if .IsResultValue }}
  {{ .Name }}() = default;

  {{- if eq 1 (len .Members) }}
  explicit {{ .Name }}({{ (index .Members 0).Type.Decl }} v) : {{ (index .Members 0).Name }}(std::move(v)) {}
  {{ (index .Members 0).Type.Decl }} ResultValue_() { return std::move({{ (index .Members 0).Name }}); }
  {{- end }}
  explicit {{ .Name }}({{ .Result.ValueTupleDecl }} _value_tuple) {
    {{- if .Result.ValueArity }}
    std::tie(
      {{- range $index, $member := .Members }}
      {{- if $index }}, {{ end -}}
      {{.Name}}
      {{- end -}}
    ) = std::move(_value_tuple);
    {{- end }}
  }
  operator {{ .Result.ValueTupleDecl }}() && {
    return std::make_tuple(
      {{- if .Result.ValueArity }}
        {{- range $index, $member := .Members }}
          {{- if $index }}, {{ end -}}
          std::move({{.Name}})
        {{- end -}}
      {{- end }}
    );
  }
  {{- end }}

  {{- range .Members }}
  {{range .DocComments}}
  ///{{ . }}
  {{- end}}
  {{ .Type.Decl }} {{ .Name }}{{ if .DefaultValue }} = {{ .DefaultValue }}{{ else }}{}{{ end }};
  {{- end }}

  static inline ::std::unique_ptr<{{ .Name }}> New() { return ::std::make_unique<{{ .Name }}>(); }

  void Encode(::fidl::Encoder* _encoder, size_t _offset);
  static void Decode(::fidl::Decoder* _decoder, {{ .Name }}* value, size_t _offset);
  zx_status_t Clone({{ .Name }}* result) const;
};

inline zx_status_t Clone(const {{ .Namespace }}::{{ .Name }}& _value,
                         {{ .Namespace }}::{{ .Name }}* _result) {
  return _value.Clone(_result);
}

using {{ .Name }}Ptr = ::std::unique_ptr<{{ .Name }}>;
{{- end }}

{{- define "StructDefinition" }}
extern "C" const fidl_type_t {{ .TableType }};
const fidl_type_t* {{ .Name }}::FidlType = &{{ .TableType }};

void {{ .Name }}::Encode(::fidl::Encoder* _encoder, size_t _offset) {
  {{- range .Members }}
  ::fidl::Encode(_encoder, &{{ .Name }}, _offset + {{ .Offset }});
  {{- end }}
}

void {{ .Name }}::Decode(::fidl::Decoder* _decoder, {{ .Name }}* value, size_t _offset) {
  {{- range .Members }}
  ::fidl::Decode(_decoder, &value->{{ .Name }}, _offset + {{ .Offset }});
  {{- end }}
}

zx_status_t {{ .Name }}::Clone({{ .Name }}* _result) const {
  {{- range $index, $member := .Members }}
  {{ if not $index }}zx_status_t {{ end -}}
  _status = ::fidl::Clone({{ .Name }}, &_result->{{ .Name }});
  if (_status != ZX_OK)
    return _status;
  {{- end }}
  return ZX_OK;
}

{{- end }}

{{- define "StructTraits" }}
template <>
struct CodingTraits<{{ .Namespace }}::{{ .Name }}>
    : public EncodableCodingTraits<{{ .Namespace }}::{{ .Name }}, {{ .InlineSize }}> {};

inline zx_status_t Clone(const {{ .Namespace }}::{{ .Name }}& value,
                         {{ .Namespace }}::{{ .Name }}* result) {
  return {{ .Namespace }}::Clone(value, result);
}

template<>
struct Equality<{{ .Namespace }}::{{ .Name }}> {
  bool operator()(const {{ .Namespace }}::{{ .Name }}& _lhs, const {{ .Namespace }}::{{ .Name }}& _rhs) const {
    {{- range $index, $member := .Members }}
    if (!::fidl::Equals(_lhs.{{ .Name }}, _rhs.{{ .Name }})) {
      return false;
    }
    {{- end }}
    return true;
  }
};

{{- end }}
`
