using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using ArduinoDriver.SerialProtocol;
using ArduinoUploader.Hardware;

namespace ArduinoDriver {
	public partial class ArduinoDAO {
		private static ArduinoDAO instance = null;

		public static ArduinoDAO Instance {
			get {
				if (null == instance)
					instance = new ArduinoDAO();

				return instance;
			}
		}
	}

	public partial class ArduinoDAO {
		private ArduinoDriver driver = null;

		public void setDriver(ArduinoModel model, string port) {
			if (null != driver) {
				driver.Dispose();
				driver = null;
			}

			driver = new ArduinoDriver(model, port);
		}

		SerialPort currentPort;
		bool portFound;

		public void SetComPort() {
			try {
				string[] ports = SerialPort.GetPortNames();
				foreach (string port in ports) {
					//currentPort = new SerialPort(port, 9600);
					tester.Send(new UserCommandRequest(0x00));
					//Thread.Sleep(1000);
					if (DetectArduino()) {
						portFound = true;
						Console.WriteLine("detected!\n");
						break;
					}
					else {
						portFound = false;
					}
				}
			}
			catch (Exception e) {
				Console.WriteLine(e.StackTrace);
			}
		}

		public bool DetectArduino() {
			try {
				goto skip;
				//The below setting are for the Hello handshake
				byte[] buffer = new byte[5];
				buffer[0] = Convert.ToByte(16);
				buffer[1] = Convert.ToByte(128);
				buffer[2] = Convert.ToByte(0);
				buffer[3] = Convert.ToByte(0);
				buffer[4] = Convert.ToByte(4);

				int intReturnASCII = 0;
				char charReturnValue = (Char)intReturnASCII;

				currentPort.Open();
				currentPort.Write(buffer, 0, 5);
				Thread.Sleep(1000);
				skip:
				int count = currentPort.BytesToRead;
				string returnMessage = "";
				while (count > 0) {
					intReturnASCII = currentPort.ReadByte();
					returnMessage = returnMessage + Convert.ToChar(intReturnASCII);
					count--;
				}
				//ComPort.name = returnMessage;

				Console.WriteLine(currentPort.PortName);

				currentPort.Close();

				if (returnMessage.Contains("HELLO FROM ARDUINO")) {
					return true;
				}
				else {
					return false;
				}
			}
			catch (Exception e) {
				return false;
			}
		}
	}

	public partial class ArduinoDAO {
		ArduinoDriver tester = new ArduinoDriver(ArduinoModel.UnoR3, "COM3");

		public void test() {
			//driver.Send(new DigitalWriteRequest(2, DigitalValue.High));
			SetComPort();
			UserCommandResponse rp1 = tester.Send(new UserCommandRequest(0x70));
			Console.ReadKey();
			//driver.Send(new DigitalWriteRequest(2, DigitalValue.Low));
			UserCommandResponse rp2 = tester.Send(new UserCommandRequest(0x60));
			Console.ReadKey();
			Console.WriteLine("success!\n");
		}
	}
}
