namespace WebInterface
{
    public class TrainProfileRepository
    {
        private List<TrainProfile> _profiles = new List<TrainProfile>()
        {
            new TrainProfile("kato-ed75", "Kato ED75", new ThrottleMap(68, 0, 155), 3, 3),
            new TrainProfile("kato-c11", "Kato C11", new ThrottleMap(15, 0, 120), 3, 3),
            new TrainProfile("tomix-de10", "Tomix DE10", new ThrottleMap(34, 0, 110), 3, 3),
        };

        public Task<IReadOnlyList<TrainProfile>> GetTrainProfilesAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult<IReadOnlyList<TrainProfile>>(_profiles.ToArray());
        }

        public Task<TrainProfile?> GetTrainProfileAsync(string id, CancellationToken cancellationToken)
        {
            return Task.FromResult(_profiles.FirstOrDefault(p => p.Id.Equals(id, StringComparison.Ordinal)));
        }
    }
}
