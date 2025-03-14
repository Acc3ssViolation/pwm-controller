namespace WebInterface
{
    public record ThrottleMap(int VMin, int VMid, int VMax);

    public record TrainProfile(string Id, string Name, ThrottleMap ThrottleMap, int Acceleration, int Braking);
}
