using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

using MotoComManager;
using static MotoComManager.Message;

using RJCP.IO;
using RJCP.IO.Ports;

namespace MotoComUTesting {
	[TestClass]
	public class MeesageTest {

		[TestInitialize]
		public void Initialization() {
			
		}

		[TestMethod]
		public void MessageValidationOne() {
			Message msg = new Message();
			msg[Field.from] = 0x5;
			msg[Field.to] = 0xD;
			msg[Field.broadcastType] = (UInt32)BroadcastType.control;
			msg[Field.senderType] = (UInt32)SenderType.soldier;
			msg[Field.messageData] = (UInt32)MessageData.stopFire;
			Assert.AreEqual((UInt32)0x12345, msg.MessageValue | BitConverter.ToUInt32(msg.MessageBytes, 0));
		}

		[TestMethod]
		public void MessageValidationTwo() {
			Message msg = new Message(0x5, 0xD, BroadcastType.control, SenderType.soldier, MessageData.stopFire);
			Assert.AreEqual((UInt32)0x12345, msg.MessageValue | BitConverter.ToUInt32(msg.MessageBytes, 0));
		}

		[TestCleanup]
		public void CleanUp() {

		}
	}
}
