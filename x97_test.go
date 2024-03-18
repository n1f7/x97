package main

import (
	"bytes"
	"testing"
)

func TestPacket(t *testing.T) {
	// $95$03$03$0B$27$24$04$16$01$5c$3c
	their := []byte{0x95, 0x03, 0x03, 0x0B, 0x27, 0x24, 0x04, 0x16, 0x01, 0x5C, 0x3C}
	pack := PreparePacket(3, []string{"0224", "96"})
	var our []byte
	our = pack.Serialize(our)

	if len(their) != int(their[3]) {
		t.Error(`Bad len`)
	}

	if !bytes.Equal(their, our) {
		t.Errorf("\n%X\n%X\n", their, our)
	}
}
