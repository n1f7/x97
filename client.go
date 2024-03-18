package main

import (
	"fmt"
	"log"
	"os"
	"strconv"

	"go.bug.st/serial"
)

func main() {
	if len(os.Args) < 2 {
		ports, err := serial.GetPortsList()
		if err != nil {
			log.Fatal(err)
		}

		if len(ports) == 0 {
			log.Fatal("No serial ports")
		}

		for _, port := range ports {
			fmt.Printf("Found: %v\n", port)
		}

	} else if 2 < len(os.Args) {
		mode := &serial.Mode{BaudRate: 115200, Parity: serial.OddParity, DataBits: 7, StopBits: serial.OneStopBit}

		addr := os.Args[1]
		cmd, _ := strconv.Atoi(os.Args[2])

		port, err := serial.Open(addr, mode)
		if err != nil {
			log.Fatal("Failure to open port: ", err)
		}

		pack := PreparePacket(cmd, os.Args[3:])
		var buf []byte
		buf = pack.Serialize(buf)

		_, err = port.Write(buf)
		if err != nil {
			log.Fatal(err)
		}

		fmt.Printf("Port: %s, Command: %d\n", addr, cmd)
		fmt.Printf("Sent %d bytes\n", len(buf))
		for _, b := range buf[:5] {
			fmt.Printf("%02X ", b)
		}
		for i, b := range buf[5 : 5+pack.CountArguments()*2] {
			if i%8 == 0 {
				fmt.Println()
			}
			fmt.Printf("%02X ", b)
		}
		fmt.Printf("\n%04X\n", pack.CRC())
	} else {
		fmt.Fprintln(os.Stderr, "Usage:\nx97 <PORT> <CMD> <ARG1> <ARG2> ... ")
	}
}
