package main

import (
	"fmt"
	"log"
	"os"
	"strconv"
	x97 "x97/proto"

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

func writeRequest(p serial.Port, cmd int) *x97.Packet {
	pack := x97.PreparePacket(cmd, os.Args[3:])
	buf := pack.Serialize(p)

	fmt.Printf("Port: %s, Command: %d\n", os.Args[1], cmd)
	fmt.Printf("Sent %d bytes\n", len(buf))

	return pack
}

func readResponse(p serial.Port) *x97.Packet {
	pack := x97.NewPacket(0)
	pack.DeSerialize(p)

	return pack
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
		cmd, _ := strconv.Atoi(os.Args[2])
		pack := writeRequest(port, cmd)
		pack.Print(os.Stdout)

		switch cmd {
		case x97.GetRegs:
			fallthrough
		case x97.SetRegsRpl:
			fallthrough
		case x97.SetRegsBitsRpl:
			fallthrough
		case x97.ExecRpl:
			fallthrough
		case x97.Read:
			fallthrough
		case x97.WriteRpl:
			pack = readResponse(port)
			pack.Print(os.Stdout)
		}
	} else {
		fmt.Fprintln(os.Stderr, "Usage:\nx97 <PORT> <CMD> <ARG1> <ARG2> ... ")
	}
}
