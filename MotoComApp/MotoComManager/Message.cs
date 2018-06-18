using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections;

namespace MotoComManager {
	class Message {
		public enum Field : UInt32 { from = 0x3F, to = 0xFC0, broadcastType = 0x3000, senderType = 0xC000, messageData = 0x70000 };

		public enum BroadcastType : UInt32 { all = 0, single = 1, control = 2, distress = 3 };
		public enum SenderType : UInt32 { soldier = 0, commander = 1, hq = 2, external = 3 };
		public enum MessageData : UInt32 { fire = 0, stopFire = 1, advance = 2, retreat = 3, ack = 4, requestID = 5, assignID = 6, isConnected = 7, verifyConnect = 8 };

		public const int messageSize = sizeof(UInt32);

		private UInt32 messageValue;
		public UInt32 MessageValue {
			get => messageValue;

			set {
				value &= 0x7FFFF;
				messageValue = value;
				BitConverter.GetBytes(value).CopyTo(MessageBytes, 0);
			}
		}
		public byte[] MessageBytes { get; set; }

		public Message(UInt32 value = 0) {
			MessageBytes = new byte[messageSize];
			MessageValue = value;
		}

		public static implicit operator byte[] (Message message) {
			return message.MessageBytes;
		}

		private int getOffset(Field field) {
			switch (field) {
				case Field.from: return 0;
				case Field.to: return 6;
				case Field.broadcastType: return 12;
				case Field.senderType: return 14;
				case Field.messageData: return 16;
				default: return -1;
			}
		}

		public UInt32 this[Field field] {
			get => (MessageValue & (UInt32)field) >> getOffset(field);

			set {
				UInt32 reminder = MessageValue & (~(UInt32)field);
				value = value << getOffset(field);
				MessageValue = reminder | (value & (UInt32)field);
			}
		}

		public override string ToString() {
			StringBuilder builder = new StringBuilder();
			builder.Append("[from: " + this[Field.from]);
			builder.Append(", to: " + this[Field.to]);
			builder.Append(", broadcastType: " + this[Field.broadcastType]);
			builder.Append(", senderType: " + this[Field.senderType]);
			builder.Append(", Data: " + this[Field.messageData] + "]");
			return builder.ToString();
		}
	}
}
