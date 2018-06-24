using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using MotoComManager;

using RJCP.IO;
using RJCP.IO.Ports;

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
		public void testSync() {
			try {
				Assert.IsTrue(driver.synchronize());
			}
			catch (Exception e) when (e is IOException || e is TimeoutException || e is NullReferenceException) {
				Assert.Fail();
			}
			finally {
			}
		}

		[TestMethod]
		public void testWrite() {
			try {
				if (driver.stream.IsOpen && driver.stream.CanWrite) {
					Message msg = new Message(0x71234);
					driver.synchronize();
					driver.writeQueue.Enqueue(msg);
					IAsyncResult result = driver.write();
					result.AsyncWaitHandle.WaitOne(500);
					Assert.IsTrue(result.IsCompleted);
				}
			}
			catch (Exception e) when (e is IOException || e is TimeoutException || e is NullReferenceException) {
				Assert.Fail();
			}
			finally {
			}
		}

		[TestMethod]
		public void testRead() {
			try {
				Message msg = null;
				driver.synchronize();
				if (driver.stream.IsOpen && driver.stream.CanRead) {// && 0 < driver.stream.BytesToRead) {
					IAsyncResult result = driver.read();
					result.AsyncWaitHandle.WaitOne(500);
					if (result.IsCompleted) {
						driver.readQueue.TryDequeue(out msg);
						Assert.IsNotNull(msg);
					}
					else
						Assert.Fail();
				}
				else {
					Assert.Inconclusive();
				}
			}
			catch (Exception e) when (e is IOException || e is TimeoutException || e is NullReferenceException) {
				Assert.Fail();
			}
			finally {
			}
		}

		[TestCleanup]
		public void CleanUp() {
			driver.Dispose();
		}
	}
}
