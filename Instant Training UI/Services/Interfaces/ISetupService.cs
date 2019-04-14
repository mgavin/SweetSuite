﻿namespace Instant.Training.UI.Services.Interfaces
{
    public interface ISetupService
    {
        bool CheckRocketLeagueInstalled();

        bool CheckBakkesModInstalled();

        void SetupPlugin();
    }
}