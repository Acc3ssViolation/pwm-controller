using Microsoft.AspNetCore.Mvc;
using System.Text.Json.Serialization;

namespace WebInterface
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateSlimBuilder(args);

            builder.Services.ConfigureHttpJsonOptions(options =>
            {
                options.SerializerOptions.TypeInfoResolverChain.Insert(0, AppJsonSerializerContext.Default);
            });

            builder.Services
                .AddSingleton<SerialService>()
                .AddSingleton<IHostedService>(sp => sp.GetRequiredService<SerialService>())
                .AddSingleton<ThrottleService>()
                .AddSingleton<IHostedService>(sp => sp.GetRequiredService<ThrottleService>())
                .AddSingleton<TrainProfileRepository>()
                ;

            var app = builder.Build();
            app.UseStaticFiles();

            var throttleApi = app.MapGroup("throttle");
            throttleApi.MapGet("/", ([FromServices] ThrottleService throttle) => throttle.Throttle);
            throttleApi.MapPut("/", ([FromServices] ThrottleService throttle, [FromBody] int value) => throttle.Throttle = value);

            throttleApi.MapGet("/enabled", ([FromServices] ThrottleService throttle) => throttle.Enabled);
            throttleApi.MapPut("/enabled", ([FromServices] ThrottleService throttle, [FromBody] bool value) => throttle.Enabled = value);

            throttleApi.MapPost("/stop", ([FromServices] ThrottleService throttle) => throttle.EmergencyStop());

            var trainProfileApi = app.MapGroup("train");
            trainProfileApi.MapGet("/", async ([FromServices] TrainProfileRepository repository, CancellationToken ct) => await repository.GetTrainProfilesAsync(ct));
            trainProfileApi.MapGet("/{id}", async ([FromServices] TrainProfileRepository repository, [FromRoute] string id, CancellationToken ct) => await repository.GetTrainProfileAsync(id, ct));

            app.Run();
        }
    }

    [JsonSerializable(typeof(IReadOnlyList<TrainProfile>))]
    [JsonSerializable(typeof(TrainProfile))]
    [JsonSerializable(typeof(bool))]
    internal partial class AppJsonSerializerContext : JsonSerializerContext
    {

    }
}
