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
			ArduinoDao.Instance.scanDevices();
			foreach (ArduinoDriver ard in ArduinoDao.Instance.drivers.Values) {
				Console.WriteLine(ard);
			}
			ArduinoDao.Instance.Dispose();
			Console.WriteLine("waiting");
			Console.ReadKey();
			Message test = new Message(0xFE72);
			ArduinoDriver myDriver = new ArduinoDriver("COM5", 9600);
			if (myDriver.synchronize()) Console.WriteLine("sync succesfull");
			while (true) {
				if (0 < myDriver.stream.BytesToRead) {
					myDriver.stream.EndRead(myDriver.read(null, null));
					myDriver.readQueue.TryDequeue(out test);
					Console.WriteLine("{0:x}: {1} :{2}", test.MessageValue, test, test.MessageValue);
				}
				else if (Console.KeyAvailable) {
					if (Console.ReadKey().Key == ConsoleKey.A) {
						Console.WriteLine("sending");
						test = new Message(0x7F000);
						//test[Message.Field.messageData] = (UInt32)Message.MessageData.retreat;
						Console.WriteLine("{0:x}: {1} :{2}", test.MessageValue, test, test.MessageValue);
						myDriver.writeQueue.Enqueue(test);
						myDriver.write();
					}
					else if (Console.ReadKey().Key == ConsoleKey.Q)
						break;
				}
			}
		}
	}
}
