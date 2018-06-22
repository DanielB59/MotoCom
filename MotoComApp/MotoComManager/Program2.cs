using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using static MotoComManager.Message;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	class Program2 {
		static void Main(string[] args) {
			Message msg = new Message();
			msg[Field.from] = 0x5;
			msg[Field.to] = 0xD;
			msg[Field.broadcastType] = (UInt32)BroadcastType.control;
			msg[Field.senderType] = (UInt32)SenderType.soldier;
			msg[Field.messageData] = (UInt32)MessageData.stopFire;
			Console.WriteLine(msg);
			byte[] bytes = new byte[sizeof(UInt32)];
			ArduinoDriver driver = new ArduinoDriver("COM5");
			driver.synchronize();
			do { while (0 >= driver.stream.BytesToRead) ;
			driver.read();
			Message msg1 = null;
			driver.readQueue.TryDequeue(out msg1);
			Console.WriteLine(msg);
			}while (true);
			return;
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
