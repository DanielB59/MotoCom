namespace ArduinoDriver.SerialProtocol {
	public class UserCommandResponse : ArduinoResponse {
		public byte Command { get; private set; }
		public int Result { get; private set; }

		public UserCommandResponse(byte command, byte result) {
			Command = command;
			Result = result;
		}
	}
}
