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
	public partial class ArduinoDao : IDisposable {
		private static ArduinoDao instance = null;

		public static ArduinoDao Instance {
			get {
				if (null == instance)
					instance = new ArduinoDao();
				return instance;
			}
		}

		private ArduinoDao() { }

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

	public partial class ArduinoDao {
		ArduinoDriver selectedDriver = null;
		string selectedPort = null;
		public Dictionary<string, ArduinoDriver> drivers = new Dictionary<string, ArduinoDriver>();
		public ObservableCollection<ArduinoDriver> viewList = new ObservableCollection<ArduinoDriver>();
	}

	public partial class ArduinoDao {
		public void enqueueMessage(Message msg) {
			selectedDriver.writeQueue.Enqueue(msg);
		}

		public void dequeueMessage(out Message msg) {
			selectedDriver.readQueue.TryDequeue(out msg);
		}

		public void scanDevices() {
			viewList.Clear();
			foreach (PortDescription port in SerialPortStream.GetPortDescriptions()) {
				ArduinoDriver driver = null;
				try {
					if (!drivers.ContainsKey(port.Port)) {
						driver = new ArduinoDriver(port.Port);
						if (driver.synchronize()) {
							drivers.Add(port.Port, driver);
							viewList.Add(driver);
						}
						else
							driver.Dispose();
					}
					else {
						driver = drivers[port.Port];
						driver.reOpen();
						if (driver.synchronize())
							viewList.Add(driver);
						else {
							drivers.Remove(port.Port);
							driver.Dispose();
						}
					}
				}
				catch {
					if (null != driver)
						driver.Dispose();
				}
			}
		}
	}
}
