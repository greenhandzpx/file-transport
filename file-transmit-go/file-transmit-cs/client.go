package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	//"time"
)

func dealFile(conn *net.TCPConn) {
	for {
		//receiveFileName(conn)
		sendFileName(conn)
	}
}

// func receiveFileName(conn *net.TCPConn) {
// 	buf := make([]byte, 1024 * 4)
// 	n, err := conn.Read(buf)
// 	if err != nil {
// 		//log.Fatal("Error when reading conn", err)
// 		return
// 	}
// 	if n == 0 {  // 一开始就是0，说明此时没有文件发过来
// 		return
// 	}
// 	fileName := string(buf[:n])
// 	fmt.Println("正在接收文件……")
// 	conn.Write([]byte("ok"))
// 	receiveFile(conn, fileName)
// }

// func receiveFile(conn *net.TCPConn, fileName string) {
// 	fileFD, err := os.Create(fileName)
// 	defer fileFD.Close()
// 	if err != nil {
// 		log.Fatal("Error when creating file", err)
// 		return
// 	}
// 	for {
// 		buf := make([]byte, 1024 * 4)
// 		n, err := conn.Read(buf)
// 		if err != nil {
// 			if err == io.EOF {
// 				fmt.Println("文件接收完毕")
// 			}
// 			log.Fatal("Error when reading conn", err)
// 			return
// 		}
// 		if n == 0 {
// 			fmt.Println("文件接收完毕")
// 			return
// 		}
// 		fileFD.Write(buf[:n])
// 	}
// }


func sendFileName(conn *net.TCPConn) {
	fmt.Println("请输入需要发送的文件的文件名:")
	var fileName string
	fmt.Scan(&fileName)
	fileState, err := os.Stat(fileName)  // 这个只是返回文件的状态，不包含文件的内容
	if err != nil {
		log.Fatal("Error when getting the fileState", err)
		return
	}
	// 先发送文件名
	_, err = conn.Write([]byte(fileState.Name()))
	fmt.Println("okk")
	if err != nil {
		log.Fatal("Error when sending file name", err)
		return
	} 
	buf := make([]byte, 1024)
	n, err := conn.Read(buf)
	if err != nil {
		log.Fatal("Error when getting the reply from client", err)
		return
	}
	if "ok" == string(buf[:n]) {
		sendFile(conn, fileName)
	}
}

func sendFile(conn *net.TCPConn, fileName string) {
	fileFD, err := os.Open(fileName)
	if err != nil {
		log.Fatal("Error when opening the file", err)
		return
	}
	defer fileFD.Close()
	buf := make([]byte, 1024*4)
	for {
		n, err := fileFD.Read(buf) // 先读文件内容到buf
		if err != nil {
			if err == io.EOF {
				fmt.Println("文件发送完毕")
			} else {
				log.Fatal("Error when reading file", err)
			}
			return
		}
		conn.Write(buf[:n]) // 再把buf的内容发过去
	}
}
func main() {
	var serverIp string
	fmt.Println("请输入服务器的ip(x.x.x.x):")
	fmt.Scan(&serverIp)
	serverAddress, err := net.ResolveTCPAddr("tcp4", serverIp+":12345")
	if err != nil {
		log.Fatal("Error when resolving", err)
		return
	}
	conn, err := net.DialTCP("tcp4", nil, serverAddress)
	if err != nil {
		log.Fatal("Error when connecting", err)
		return
	}
	defer conn.Close()
	dealFile(conn)
}