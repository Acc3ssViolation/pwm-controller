using System.IO.Ports;
using System.Text;

namespace WebInterface
{
    public class ControllerInterface : IDisposable
    {
        private readonly StringBuilder _sb = new StringBuilder();
        private readonly SerialPort _port;
        private bool _disposed;

        public ControllerInterface(SerialPort port)
        {
            _port = port;
            _port.BaudRate = 115200;
            _port.Parity = Parity.None;
            _port.StopBits = StopBits.One;
            _port.Handshake = Handshake.None;
            _port.RtsEnable = false;
            _port.ReadTimeout = 100;
            _port.WriteTimeout = 100;
            _port.WriteBufferSize = 128;
            _port.ReadBufferSize = 128;
            _port.Encoding = Encoding.ASCII;
            _port.NewLine = "\r\n";
        }

        public bool SendCommand(string msg)
        {
            try
            {
                if (!_port.IsOpen)
                {
                    _port.Open();
                }
                _port.WriteLine(msg);
                _sb.Append(_port.ReadExisting());
                _port.DiscardInBuffer();
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                return false;
            }
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    _port.Dispose();
                }

                _disposed = true;
            }
        }

        public void Dispose()
        {
            // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
