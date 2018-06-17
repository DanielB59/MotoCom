using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	class Program2 {
		static void Main(string[] args) {
			goto skip;
			byte[] bytes = null;
			UInt32 val = 0xFE72;
			ArduinoDriver driver = new ArduinoDriver("COM3", 9600);
			bytes = BitConverter.GetBytes(val);
			driver.stream.Write(bytes, 0, sizeof(UInt32));
			driver.stream.Read(bytes, 0, sizeof(UInt32));
			val = BitConverter.ToUInt32(bytes, 0);
			Console.WriteLine(val);
			if (0xFe72 == val)
				Console.WriteLine("equals!");
			Console.ReadKey();
			skip:
			goto skip2;
			ArduinoDao.Instance.scanDevices();
			foreach (PortDescription port in ArduinoDao.Instance.drivers.Keys) {
				Console.WriteLine(port.Port + " | " + port.Description);
			}
			skip2:
			goto skip3;
			ArduinoDriver driver2 = new ArduinoDriver("COM3", 9600);
			for (int i = 0; i < 20; i++) {
				driver2.read();
				//Thread.Sleep(200);
			}
			//Console.ReadKey();
			Message temp;
			foreach (Message msg in driver2.readQueue) Console.WriteLine("{0}\t{1:x}", msg, msg.MessageValue);
			if (driver2.readQueue.IsEmpty) Console.WriteLine("empty");
			while (!driver2.readQueue.IsEmpty) driver2.readQueue.TryDequeue(out temp);
			Thread.Sleep(333);
			Console.WriteLine("derp");
			for (int i = 0; i < 20; i++) {
				driver2.read();
				//Thread.Sleep(200);
			}
			foreach (Message msg in driver2.readQueue) Console.WriteLine("{0}\t{1:x}", msg, msg.MessageValue);
			Console.ReadKey();
			skip3:
			Message test = new Message();
			test.MessageValue = 0x78;
			byte[] buff = new byte[Message.messageSize];
			buff = BitConverter.GetBytes(0x78);
			SerialPortStream sps = new SerialPortStream("COM3", 9600);
			sps.Open();
			//sps.Write(test, 0, Message.messageSize);
			int j = 0;
			while (j++ < 20) sps.EndWrite(sps.BeginWrite(test.MessageBytes, 0, Message.messageSize, null, null));
			//test.MessageValue = 0x68;
			//sps.Write(test, 0, Message.messageSize);
			Console.ReadKey();
			sps.Close();
		}
	}
}
