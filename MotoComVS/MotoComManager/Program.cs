using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	class Program {
		static void Main(string[] args) {
			goto skip;
			byte[] bytes = null;
			UInt32 val = 0xFe72;
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
			ArduinoDao.Instance.scanDevices();
			foreach(PortDescription port in ArduinoDao.Instance.drivers.Keys) {
				Console.WriteLine(port.Port + " | " + port.Description);
			}
		}
	}
}
