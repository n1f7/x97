package main

import (
	"bytes"
	"testing"
)

func TestPacket(t *testing.T) {
	// cmd == 3, len == 15,
	// $95$03$03$0B$27$24$04$16$01$5c$3c
	their := []byte{0x95, 0x03, 0x03, 0x0B, 0x27, 0x24, 0x04, 0x16, 0x01, 0x5C, 0x3C}
	our := PreparePacket(3, []string{"0224", "96"})

	if len(their) != int(their[3]) {
		t.Error(`Bad len`)
	}

	if !bytes.Equal(our, their) {
		t.Error(`Wrong`)
	}
}
