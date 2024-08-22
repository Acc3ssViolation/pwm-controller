
using System.Threading.Channels;

namespace WebInterface
{
    public class SerialService : IHostedService
    {
        private readonly ILogger<SerialService> _logger;
        private readonly Channel<string> _commands = Channel.CreateBounded<string>(25);
        private Thread? _threadHandle;
        private CancellationTokenSource? _cts;

        public SerialService(ILogger<SerialService> logger)
        {
            _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        }

        private void SerialThread(object? argument)
        {
            ArgumentNullException.ThrowIfNull(argument);

            CancellationToken stoppingToken = (CancellationToken)argument;
            using var port = new ControllerInterface(new System.IO.Ports.SerialPort("COM3"));
            
            //port.SendCommand("ECHO ON");

            while (!stoppingToken.IsCancellationRequested)
            {
                try
                {
                    var command = Task.Run(async () => await _commands.Reader.ReadAsync(stoppingToken)).Result;
                    _logger.LogDebug("Sending command: '{Command}'", command);
                    port.SendCommand(command);
                }
                catch (OperationCanceledException)
                {
                    // Exit
                }
                catch (Exception ex) when (!stoppingToken.IsCancellationRequested)
                {
                    _logger.LogError(ex, "Exception in serial thread");
                }
            }
        }

        public bool EnqueueCommand(string command)
        {
            return _commands.Writer.TryWrite(command);
        }

        public Task StartAsync(CancellationToken cancellationToken)
        {
            _cts = new CancellationTokenSource();
            _threadHandle = new Thread(SerialThread);
            _threadHandle.Start(_cts.Token);
            return Task.CompletedTask;
        }

        public Task StopAsync(CancellationToken cancellationToken)
        {
            _cts?.Cancel();
            return Task.CompletedTask;
        }
    }
}
