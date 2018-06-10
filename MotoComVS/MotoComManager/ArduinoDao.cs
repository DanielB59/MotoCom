using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	partial class ArduinoDao: IDisposable {
		private static ArduinoDao instance = null;

		public static ArduinoDao Instance {
			get {
				if (null == instance)
					instance = new ArduinoDao();
				return instance;
			}
		}

		public void Dispose() {
			foreach (ArduinoDriver driver in drivers.Values)
				driver.Dispose();
		}
	}

	partial class ArduinoDao {
		ArduinoDriver selectedDriver = null;
		PortDescription selectedPort = null;
		public Dictionary<PortDescription, ArduinoDriver> drivers = new Dictionary<PortDescription, ArduinoDriver>();
	}

	partial class ArduinoDao {
		public void scanDevices() {
			foreach (PortDescription port in SerialPortStream.GetPortDescriptions()) {
				ArduinoDriver driver = new ArduinoDriver(port.Port);
				if (driver.synchronize())
					drivers.Add(port, driver);
			}
		}
	}
}
