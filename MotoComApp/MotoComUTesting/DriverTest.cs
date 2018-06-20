using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using MotoComManager;

namespace MotoComUTesting {
	[TestClass]
	public class DriverTest {

		const string port = "COM5";
		const int baud = 9600;

		ArduinoDriver driver;

		[TestInitialize]
		public void Initialization() {
			driver = new ArduinoDriver(port, baud);
		}

		[TestMethod]
		public void syncTest() {
			Assert.IsTrue(driver.synchronize());
		}

		[TestMethod]
		public void testWrite() {
			try {
				
			}
			catch {

			}
			finally {
					
			}
		}

		[TestMethod]
		public void testRead() {

		}

		[TestCleanup]
		public void CleanUp() {

		}
	}
}
