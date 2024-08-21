
namespace WebInterface
{
    public class ThrottleService : BackgroundService
    {
        public static readonly int MinThrottle = -255;
        public static readonly int MaxThrottle = 255;

        private readonly SerialService _serialService;
        private readonly ILogger<ThrottleService> _logger;

        private readonly object _lock = new();

        private int _throttle;
        private bool _enabled = true;

        public int Throttle
        {
            get
            {
                return _throttle;
            }
            set
            {
                ArgumentOutOfRangeException.ThrowIfLessThan(value, MinThrottle);
                ArgumentOutOfRangeException.ThrowIfGreaterThan(value, MaxThrottle);

                lock (_lock)
                {
                    _throttle = value;
                }
            }
        }

        public bool Enabled
        {
            get
            {
                return _enabled;
            }
            set
            {
                if (value == _enabled)
                    return;

                _enabled = value;
                _throttle = 0;
                if (!value)
                {
                    _serialService.EnqueueCommand("PC OFF");
                }
            }
        }

        public ThrottleService(SerialService serialService, ILogger<ThrottleService> logger)
        {
            _serialService = serialService ?? throw new ArgumentNullException(nameof(serialService));
            _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                try
                {
                    lock (_lock)
                    {
                        if (_enabled)
                        {
                            var absThrottle = Math.Abs(_throttle);
                            _serialService.EnqueueCommand("PC ON");
                            if (_throttle >= 0)
                            {
                                _serialService.EnqueueCommand($"DC FWD {absThrottle}");
                            }
                            else
                            {
                                _serialService.EnqueueCommand($"DC REV {absThrottle}");
                            }
                        }
                        else
                        {
                            _serialService.EnqueueCommand("PC OFF");
                        }

                    }

                    await Task.Delay(TimeSpan.FromMilliseconds(1000), stoppingToken).ConfigureAwait(false);
                }
                catch (OperationCanceledException)
                {
                }
                catch (Exception ex)
                {
                    _logger.LogError(ex, "Exception in ThrottleService");
                }
            }
        }

        public void EmergencyStop()
        {
            lock (_lock )
            {
                _throttle = 0;
                _serialService.EnqueueCommand("DC STOP");
            }
        }
    }
}
