using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComManager {
	partial class ArduinoDao : IDisposable {
		private static ArduinoDao instance = null;

		public static ArduinoDao Instance {
			get {
				if (null == instance)
					instance = new ArduinoDao();
				return instance;
			}
		}

		private ArduinoDao() {
			//ItemsSource = drivers;
		}

		~ArduinoDao() => Dispose(false);

		#region IDisposable Support
		private bool disposedValue = false;

		protected virtual void Dispose(bool disposing) {
			if (!disposedValue) {
				if (disposing) {
					foreach (ArduinoDriver driver in drivers.Values)
						driver.Dispose();
				}
				
				disposedValue = true;
			}
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}
		#endregion
	}

	partial class ArduinoDao {
		ArduinoDriver selectedDriver = null;
		PortDescription selectedPort = null;
		public Dictionary<PortDescription, ArduinoDriver> drivers = new Dictionary<PortDescription, ArduinoDriver>();
		//ObservableCollection<KeyValuePair<PortDescription, ArduinoDriver>> coll = new ObservableCollection<KeyValuePair<PortDescription, ArduinoDriver>>(drivers);
	}

	partial class ArduinoDao {//: System.Windows.Controls.ItemsControl {
		public void scanDevices() {
			foreach (PortDescription port in SerialPortStream.GetPortDescriptions()) {
				ArduinoDriver driver = new ArduinoDriver(port.Port);
				if (driver.synchronize())
					drivers.Add(port, driver);
			}
		}
	}
}
