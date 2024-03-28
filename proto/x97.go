package x97

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
)

const (
	GetRegs = iota + 1
	SetRegs
	SetRegsRpl
	SetRegsBits
	SetRegsBitsRpl
	Exec
	ExecRpl
	Read = iota + 9
	Write
	WriteRpl
)

var CRC16Table = [...]uint16{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040}

func crc16(data []byte) uint16 {
	var reg_crc uint16 = 0xFFFF

	for i := 0; i < len(data); i++ {
		temp := uint8(uint16(data[i]) ^ reg_crc)
		reg_crc >>= 8
		reg_crc ^= CRC16Table[temp]
	}
	return reg_crc
}

func crc_header(data []byte) uint8 {
	var sum uint
	for i := 0; i < len(data); i++ {
		sum += uint(data[i])
	}
	return uint8((sum>>7)+sum) & 0x7f
}

func crc_pack(data []byte) uint16 {
	crc := crc16(data)
	return ((crc >> 14) + crc) & 0x3fff
}

func sharoword(x uint16) uint16 {
	return ((x & 0x3F80) << 1) | (x & 0x7F)
}

type Packet struct {
	data [5 + 127]byte
}

func NewPacket(command int) *Packet {
	var pack Packet
	if 0 < command {
		pack.SetId(0x95)
		pack.SetAddr(3)
		pack.SetCommand(uint8(command))
		pack.SetLength(5)
	}
	return &pack
}

func (x *Packet) CountArguments() int {
	if 0 < x.Length()-5 {
		return (int(x.Length()) - 7) / 2
	} else {
		return 0
	}
}

func (x *Packet) Header() []byte {
	return x.data[:5]
}

func (x *Packet) Data() []byte {
	return x.data[5 : x.Length()-2]
}

func (x *Packet) SetId(id uint8) {
	x.data[0] = id
}

func (x *Packet) Id() uint8 {
	return x.data[0]
}

func (x *Packet) SetAddr(addr uint8) {
	x.data[1] = addr
}

func (x *Packet) Addr() uint8 {
	return x.data[1]
}

func (x *Packet) SetCommand(cmd uint8) {
	x.data[2] = cmd
}

func (x *Packet) Command() uint8 {
	return x.data[2]
}

func (x *Packet) SetLength(length uint8) {
	x.data[3] = length
}

func (x *Packet) Length() uint8 {
	return x.data[3]
}

func (x *Packet) SetHeaderCRC(crc uint8) {
	x.data[4] = crc
}

func (x *Packet) HeaderCRC() uint8 {
	return x.data[4]
}

func (x *Packet) Print(out io.Writer) {
	for _, b := range x.Header() {
		fmt.Fprintf(out, "%02X ", b)
	}
	for i, b := range x.Data() {
		if i%8 == 0 {
			fmt.Fprintln(out)
		}
		fmt.Fprintf(out, "%02X ", b)
	}
	fmt.Fprintf(out, "\n%04X\n", x.CRC())
}

func (x *Packet) AppendArguments(arguments []string) {
	buf := new(bytes.Buffer)
	for _, s := range arguments {
		arg, err := strconv.ParseInt(s, 16, 16)
		if err != nil {
			log.Fatal("Fail to parse command arguments ", err)
		}
		binary.Write(buf, binary.LittleEndian, sharoword(uint16(arg)))
	}

	sz, _ := buf.Read(x.data[5:])
	if 0 < sz {
		sz += 2
	}
	sz += 5
	x.SetLength(uint8(sz))
}

func (x *Packet) CRC() uint16 {
	return uint16(x.data[x.Length()-2]) | uint16(x.data[x.Length()-1])<<8
}

func (x *Packet) SetCRC(crc uint16) {
	x.data[x.Length()-2] = uint8(crc >> 0)
	x.data[x.Length()-1] = uint8(crc >> 8)
}

func (x *Packet) ComputeCRC() {
	x.SetHeaderCRC(crc_header(x.data[:4]))

	if 5 < x.Length() {
		x.SetCRC(sharoword(crc_pack(x.data[:x.Length()-2])))
	}
}

func (x *Packet) Serialize(out io.Writer) []byte {
	l, err := out.Write(x.data[:x.Length()])
	if err != nil {
		log.Fatal("failed to write: ", err)
	}
	return x.data[:l]
}

func (x *Packet) isValid() bool {
	// TODO: verify CRC
	return x.Id() == 0x95 && 0 < x.Length()
}

func (x *Packet) DeSerialize(in io.Reader) []byte {
	for limit, sz, received := -1, 0, 0; ; {
		var err error = nil

		if 0 < limit {
			sz, err = in.Read(x.data[received:limit])
		} else {
			sz, err = in.Read(x.data[received:])
		}

		received += sz

		if sz == 0 {
			break
		} else if err != nil {
			fmt.Fprintln(os.Stderr, "Failure to rcv: ", err)
			break
		} else if limit < 0 && 4 < received && x.isValid() { // Reading body
			limit = int(x.Length())
		}
	}

	return x.data[:x.Length()]
}

func PreparePacket(cmd int, args []string) *Packet {
	pack := NewPacket(cmd)
	pack.AppendArguments(args)
	pack.ComputeCRC()
	return pack
}
