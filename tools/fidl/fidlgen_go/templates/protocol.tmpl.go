// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

const Protocol = `
{{- define "ProtocolDefinition" -}}

const (
{{- range .Methods }}
	{{- range .Ordinals.Reads }}
	{{ .Name }} uint64 = {{ .Ordinal }}
	{{- end }}
{{- end }}
)

type {{ .ProxyName }} _bindings.{{ .ProxyType }}
{{ range .Methods }}
{{range .DocComments}}
//{{ . }}
{{- end}}
func (p *{{ $.ProxyName }}) {{ if .IsEvent -}}
		{{ .EventExpectName }}
	{{- else -}}
		{{ .Name }}
	{{- end -}}
	(ctx_ _bindings.Context
	{{- if .Request -}}
	{{- range .Request.Members -}}
		, {{ .PrivateName }} {{ .Type }}
	{{- end -}}
	{{- end -}}
	)
	{{- if .HasResponse -}}
	{{- if .Response }} (
	{{- range .Response.Members }}{{ .Type }}, {{ end -}}
		error)
	{{- else }} error{{ end -}}
	{{- else }} error{{ end }} {

	{{- if .HasRequest }}
	{{- if .Request }}
	req_ := &{{ .Request.Name }}{
		{{- range .Request.Members }}
		{{ .Name }}: {{ .PrivateName }},
		{{- end }}
	}
	{{- else }}
	var req_ _bindings.Message
	{{- end }}
	{{- end }}
	{{- if .HasResponse }}
	{{- if .Response }}
	resp_ := &{{ .Response.Name }}{}
	{{- else }}
	var resp_ _bindings.Message
	{{- end }}
	{{- end }}
	{{- if .HasRequest }}
		{{- if .HasResponse }}
	err_ := ((*_bindings.{{ $.ProxyType }})(p)).Call({{ .Ordinals.Write.Name }}, req_, resp_
		{{- range $index, $ordinal := .Ordinals.Reads -}}, {{ $ordinal.Name }}{{- end -}})
		{{- else }}
	err_ := ((*_bindings.{{ $.ProxyType }})(p)).Send({{ .Ordinals.Write.Name }}, req_)
		{{- end }}
	{{- else }}
		{{- if .HasResponse }}
	err_ := ((*_bindings.{{ $.ProxyType }})(p)).Recv(
		{{- with $first_ordinal := index .Ordinals.Reads 0 -}}
			{{- $first_ordinal.Name -}}
		{{- end -}}
		, resp_
		{{- range $index, $ordinal := .Ordinals.Reads -}}
			{{- if $index -}}, {{ $ordinal.Name }}{{- end -}}
		{{- end -}}
		)
		{{- else }}
	err_ := nil
		{{- end }}
	{{- end }}
	{{- if .HasResponse }}
	{{- if .Response }}
	return {{ range .Response.Members }}resp_.{{ .Name }}, {{ end }}err_
	{{- else }}
	return err_
	{{- end }}
	{{- else }}
	return err_
	{{- end }}
}
{{- end }}

{{range .DocComments}}
//{{ . }}
{{- end}}
type {{ .Name }} interface {
{{- range .Methods }}
	{{- range .DocComments}}
	//{{ . }}
	{{- end}}
	{{- if .HasRequest }}
	{{- if .Request }}
	{{ .Name }}(ctx_ _bindings.Context
	{{- range .Request.Members -}}
		, {{ .PrivateName }} {{ .Type }}
	{{- end -}}
	)
	{{- else }}
	{{ .Name }}(ctx_ _bindings.Context)
	{{- end }}
	{{- if .HasResponse -}}
	{{- if .Response }} (
	{{- range .Response.Members }}{{ .Type }}, {{ end -}}
		error)
	{{- else }} error{{ end -}}
	{{- else }} error{{ end }}
	{{- end }}
{{- end }}
}

{{ $transitionalBaseName := .TransitionalBaseName }}

type {{.TransitionalBaseName}} struct {}

{{ range .Methods }}
{{- if and .IsTransitional .HasRequest }}
{{- if .Request }}
func (_ *{{$transitionalBaseName}}) {{ .Name }} (ctx_ _bindings.Context
{{- range .Request.Members -}}
	, {{ .PrivateName }} {{ .Type }}
{{- end -}}
)
{{- else }}
func (_ *{{$transitionalBaseName}}) {{ .Name }} (ctx_ _bindings.Context)
{{- end }}
{{- if .HasResponse -}}
	{{- if .Response }} (
		{{- range .Response.Members }}{{ .Type }}, {{ end -}}
			error)
	{{- else -}}
		error
	{{ end -}}
{{- else -}}
	error
{{- end -}}
{
	panic("Not Implemented")
}
{{- end }}
{{- end }}

{{- if eq .ProxyType "ChannelProxy" }}
type {{ .RequestName }} _bindings.InterfaceRequest

func New{{ .RequestName }}() ({{ .RequestName }}, *{{ .ProxyName }}, error) {
	req, cli, err := _bindings.NewInterfaceRequest()
	return {{ .RequestName }}(req), (*{{ .ProxyName }})(cli), err
}

{{- if .ServiceNameString }}
// Implements ServiceRequest.
func (_ {{ .RequestName }}) Name() string {
	return {{ .ServiceNameString }}
}
func (c {{ .RequestName }}) ToChannel() _zx.Channel {
	return c.Channel
}

const {{ .ServiceNameConstant }} = {{ .ServiceNameString }}
{{- end }}
{{- end }}

type {{ .StubName }} struct {
	Impl {{ .Name }}
}

func (s_ *{{ .StubName }}) Dispatch(args_ _bindings.DispatchArgs) (_bindings.Message, bool, error) {
	switch args_.Ordinal {
	{{- range .Methods }}
	{{- if not .IsEvent }}
	{{- range $index, $ordinal := .Ordinals.Reads }}
		{{- if $index }}
		fallthrough
		{{- end }}
	case {{ $ordinal.Name }}:
	{{- end }}
		{{- if .HasRequest }}
		{{- if .Request }}
		in_ := {{ .Request.Name }}{}
		marshalerCtx, ok := _bindings.GetMarshalerContext(args_.Ctx)
		if !ok {
			return nil, false, _bindings.ErrMissingMarshalerContext
		}
		if _, _, err_ := _bindings.UnmarshalWithContext2(marshalerCtx, args_.Bytes, args_.HandleInfos, &in_); err_ != nil {
			return nil, false, err_
		}
		{{- end }}
		{{- end }}
		{{ if .Response }}
		{{- range .Response.Members }}{{ .PrivateName }}, {{ end -}}
		{{- end -}}
		err_ := s_.Impl.{{ .Name }}(args_.Ctx
		{{- if .HasRequest -}}
		{{- if .Request -}}
		{{- range $index, $m := .Request.Members -}}
		, in_.{{ $m.Name }}
		{{- end -}}
		{{- end -}}
		{{- end -}}
		)
		{{- if .HasResponse }}
		{{- if .Response }}
		out_ := {{ .Response.Name }}{}
		{{- range .Response.Members }}
		out_.{{ .Name }} = {{ .PrivateName }}
		{{- end }}
		return &out_, true, err_
		{{- else }}
		return nil, true, err_
		{{- end }}
		{{- else }}
		return nil, false, err_
		{{- end }}
	{{- end }}
	{{- end }}
	}
	return nil, false, _bindings.ErrUnknownOrdinal
}

{{- if eq .ProxyType "ChannelProxy" }}
type {{ .ServerName }} struct {
	_bindings.BindingSet
}

func (s *{{ .ServerName }}) EventProxyFor(key _bindings.BindingKey) (*{{ .EventProxyName }}, bool) {
	pxy, err := s.BindingSet.ProxyFor(key)
	return (*{{ .EventProxyName }})(pxy), err
}
{{- end }}

type {{ .EventProxyName }} _bindings.{{ .ProxyType }}
{{ range .Methods }}
{{- if .IsEvent }}
func (p *{{ $.EventProxyName }}) {{ .Name }}(
	{{- if .Response -}}
	{{- range $index, $m := .Response.Members -}}
		{{- if $index -}}, {{- end -}}
		{{ $m.PrivateName }} {{ $m.Type }}
	{{- end -}}
	{{- end -}}
	) error {

	{{- if .HasResponse }}
	{{- if .Response }}
	event_ := &{{ .Response.Name }}{
		{{- range .Response.Members }}
		{{ .Name }}: {{ .PrivateName }},
		{{- end }}
	}
	{{- else }}
	var event_ _bindings.Message
	{{- end }}
	{{- end }}
	return ((*_bindings.{{ $.ProxyType }})(p)).Send({{ .Ordinals.Write.Name }}, event_)
}
{{- end }}
{{- end }}

{{ end -}}
`
