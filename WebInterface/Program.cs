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
                ;

            var app = builder.Build();
            app.UseStaticFiles();

            var sampleTodos = new Todo[] {
                new(1, "Walk the dog"),
                new(2, "Do the dishes", DateOnly.FromDateTime(DateTime.Now)),
                new(3, "Do the laundry", DateOnly.FromDateTime(DateTime.Now.AddDays(1))),
                new(4, "Clean the bathroom"),
                new(5, "Clean the car", DateOnly.FromDateTime(DateTime.Now.AddDays(2)))
            };

            var throttleApi = app.MapGroup("throttle");
            throttleApi.MapGet("/", ([FromServices] ThrottleService throttle) => throttle.Throttle);
            throttleApi.MapPut("/", ([FromServices] ThrottleService throttle, [FromBody] int value) => throttle.Throttle = value);

            throttleApi.MapGet("/enabled", ([FromServices] ThrottleService throttle) => throttle.Enabled);
            throttleApi.MapPut("/enabled", ([FromServices] ThrottleService throttle, [FromBody] bool value) => throttle.Enabled = value);

            throttleApi.MapPost("/stop", ([FromServices] ThrottleService throttle) => throttle.EmergencyStop());

            var todosApi = app.MapGroup("/todos");
            todosApi.MapGet("/", () => sampleTodos);
            todosApi.MapGet("/{id}", (int id) =>
                sampleTodos.FirstOrDefault(a => a.Id == id) is { } todo
                    ? Results.Ok(todo)
                    : Results.NotFound());

            app.Run();
        }
    }

    public record Todo(int Id, string? Title, DateOnly? DueBy = null, bool IsComplete = false);

    [JsonSerializable(typeof(Todo[]))]
    internal partial class AppJsonSerializerContext : JsonSerializerContext
    {

    }
}
