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
			msg[Field.from] = 0x16;
			msg[Field.to] = 0x11;
			msg[Field.broadcastType] = (UInt32)BroadcastType.distress;
			msg[Field.senderType] = (UInt32)SenderType.soldier;
			msg[Field.messageData] = (UInt32)MessageData.ack;
			msg[Field.clusterID] = 0xC;
			Assert.AreEqual((UInt32)0x3043456, msg.MessageValue | BitConverter.ToUInt32(msg.MessageBytes, 0));
		}

		[TestMethod]
		public void MessageValidationTwo() {
			Message msg = new Message(0x16, 0x11, BroadcastType.distress, SenderType.soldier, MessageData.ack, 0xC);
			Assert.AreEqual((UInt32)0x3043456, msg.MessageValue | BitConverter.ToUInt32(msg.MessageBytes, 0));
		}

		[TestCleanup]
		public void CleanUp() {

		}
	}
}
