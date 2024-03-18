package main

import (
	"fmt"
	"log"
	"os"
	"strconv"

	"go.bug.st/serial"
)

func setupCOM() serial.Port {
	mode := &serial.Mode{BaudRate: 115200, Parity: serial.OddParity, DataBits: 7, StopBits: serial.OneStopBit}

	port, err := serial.Open(os.Args[1], mode)
	if err == nil {
		return port
	} else {
		log.Fatal("Failure to open port: ", err)
	}

	return port
}

func writeRequest(p serial.Port) {
	cmd, _ := strconv.Atoi(os.Args[2])
	pack := PreparePacket(cmd, os.Args[3:])
	buf := pack.Serialize(p)

	fmt.Printf("Port: %s, Command: %d\n", os.Args[1], cmd)
	fmt.Printf("Sent %d bytes\n", len(buf))
	for _, b := range pack.Header() {
		fmt.Printf("%02X ", b)
	}
	for i, b := range pack.Data() {
		if i%8 == 0 {
			fmt.Println()
		}
		fmt.Printf("%02X ", b)
	}
	fmt.Printf("\n%04X\n", pack.CRC())
}

func readResponse(p serial.Port) {
	pack := NewX97Packet(0)
	pack.DeSerialize(p)
}

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
		port := setupCOM()
		writeRequest(port)
		readResponse(port)
	} else {
		fmt.Fprintln(os.Stderr, "Usage:\nx97 <PORT> <CMD> <ARG1> <ARG2> ... ")
	}
}
