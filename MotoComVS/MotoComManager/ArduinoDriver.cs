using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	partial class ArduinoDriver {
		string port;
		int baud;
		SerialPortStream stream;

		ConcurrentQueue<byte[]> readQueue;
		ConcurrentQueue<byte[]> writeQueue;
	}

	partial class ArduinoDriver {
		public ArduinoDriver(string port, int baud = -1) {
			this.port = port;

			if (-1 == baud) {
				stream = new SerialPortStream(port);
				stream.Open();
				stream.GetPortSettings();
				baud = stream.BaudRate;
			}
			else {
				this.baud = baud;
				stream = new SerialPortStream(port, baud);
				stream.Open();
			}

			sync();
		}

		~ArduinoDriver() => this.Dispose();
	}

	partial class ArduinoDriver : IDisposable {
		public void Dispose() {
			try {
				stream.Close();
			}
			catch {
				//TODO: error handling
			}
		}
	}

	partial class ArduinoDriver {
		public void sync() {

		}

		public void read(AsyncCallback callback) {
			//IAsyncResult res = stream.BeginRead();
		}
	}
}
