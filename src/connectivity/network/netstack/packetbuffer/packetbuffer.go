// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package packetbuffer

import (
	"fmt"

	"gvisor.dev/gvisor/pkg/tcpip/buffer"
	"gvisor.dev/gvisor/pkg/tcpip/stack"
)

// OutboundToInbound coverts a PacketBuffer that conforms to the
// contract of WritePacket to one that conforms to the contract of
// DeliverNetworkPacket/InjectInbound/etc.
//
// The contract of WritePacket differs from that of DeliverNetworkPacket.
// WritePacket treats Header and Data as disjoint buffers; DeliverNetworkPacket
// expects Data to contain the full packet, including any Header bytes if they
// are present.
func OutboundToInbound(pkt stack.PacketBuffer) stack.PacketBuffer {
	if hdr := pkt.Header.View(); len(hdr) > 0 {
		pkt.Data = buffer.NewVectorisedView(len(hdr)+pkt.Data.Size(), append([]buffer.View{hdr}, pkt.Data.Views()...))
		pkt.Header = buffer.Prependable{}
	}
	return pkt
}

// EnsurePopulatedHeader ensures a packet buffer has a populated Header field.
//
// If the Header is unused, the packet buffer's Data will will be copied to
// Header while leaving maxLinkHeaderLen bytes available. Otherwise, the packet
// buffer will not be modified.
func EnsurePopulatedHeader(pkt *stack.PacketBuffer, maxLinkHeaderLen int) {
	if pkt.Header.UsedLength() != 0 {
		return
	}

	dataView := pkt.Data.ToView()
	if maxLinkHeaderLen == 0 {
		pkt.Header = buffer.NewPrependableFromView(dataView)
	} else {
		dataSize := len(dataView)
		pkt.Header = buffer.NewPrependable(maxLinkHeaderLen + dataSize)
		if n := copy(pkt.Header.Prepend(dataSize), dataView); n != dataSize {
			panic(fmt.Sprintf("copied %d bytes, expected %d bytes", n, dataSize))
		}
	}
	pkt.Data = buffer.VectorisedView{}
}
