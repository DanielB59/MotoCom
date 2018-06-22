using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using MotoComManager;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComUTesting {
	[TestClass]
	public class DaoTest {

		ArduinoDao dao;

		[TestInitialize]
		public void Initialization() {
			dao = ArduinoDao.Instance;
		}

		[TestMethod]
		public void testDeviceScan() {
			try {
				dao.scanDevices();
				if (0 < dao.drivers.Count)
					Assert.IsTrue(true);
				else
					Assert.Inconclusive();
			}
			catch (Exception e) when (e is IOException || e is TimeoutException || e is NullReferenceException) {
				Assert.Fail();
			}
			finally {
			}
		}

		[TestCleanup]
		public void CleanUp() {
			dao.Dispose();
		}
	}
}
