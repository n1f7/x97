package x97

import (
	"bytes"
	"testing"
)

func TestPacket(t *testing.T) {
	// $95$03$03$0B$27$24$04$16$01$5c$3c
	their := []byte{0x95, 0x03, 0x03, 0x0B, 0x27, 0x24, 0x04, 0x16, 0x01, 0x06, 0x6B}

	if len(their) != int(their[3]) {
		t.Error(`Bad len`)
	}

	pack := PreparePacket(3, []string{"0224", "96"})
	var our bytes.Buffer
	pack.Serialize(&our)

	if !bytes.Equal(their, our.Bytes()) {
		t.Errorf("\n%X\n%X\n", their, our)
	}
}

func TestPacketPrint(t *testing.T) {
	their := "95 03 03 0B 27 \n24 04 16 01 \n6B06\n"
	pack := PreparePacket(3, []string{"0224", "96"})

	var buf bytes.Buffer
	pack.Print(&buf)

	if buf.String() != their {
		t.Errorf("Bad print\n%s...\n%s...", buf.String(), their)
	}
}
