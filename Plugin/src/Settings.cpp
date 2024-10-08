#include "Settings.h"

#include "Hooks.h"
#include "Utils.h"

#define ICON_FK_UNDO reinterpret_cast<const char*>(u8"\uf0e2")

namespace Settings
{
	std::string EnumStepper::GetStepperText(int32_t a_value) const
    {
		if (optionNames.size() > a_value) {
			return optionNames[a_value];
		}

		return "Invalid";
    }

    std::string ValueStepper::GetStepperText(int32_t a_value) const
    {
		return std::to_string(GetValueFromStepper(a_value));
    }

    float Slider::GetSliderPercentage() const
    {
		return (value.get_data() - sliderMin) / (sliderMax - sliderMin);
    }

    std::string Slider::GetSliderText() const
    {
		return std::format("{:.0f}{}", value.get_data(), suffix);
    }

    float Slider::GetValueFromSlider(float a_percentage) const
    {
		return std::roundf(a_percentage * (sliderMax - sliderMin) + sliderMin);
    }

    void Slider::SetValueFromSlider(float a_percentage)
    {
        *value = GetValueFromSlider(a_percentage);
    }

    bool Main::InitCompatibility(RE::BGSSwapChainObject* a_swapChainObject)
	{
		// NOTE: this is called every time the game switches between windowed and borderless.
		// It's not called again when we replace swapchain color space or format ourselves.

		swapChainObject = a_swapChainObject;

		// force fGamma and fGammaUI to 2.4 just in case, even though we ignore it anyway
        *Offsets::fGamma = 2.4f;
        *Offsets::fGammaUI = 2.4f;

		// check for other plugins being present
		auto isModuleLoaded = [&](LPCWSTR a_moduleName) {
			HMODULE hModule = nullptr;
			GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, a_moduleName, &hModule);
			return hModule != nullptr;
		};

		constexpr std::array moduleNamesNativeHDR = { L"NativeHDR.dll", L"NativeHDR.asi", L"NativeAutoHDR.dll", L"NativeAutoHDR.asi" };
		constexpr std::array moduleNamesShaderInjector = { L"SFShaderInjector.dll", L"SFShaderInjector.asi" };
		constexpr auto moduleNameDLSSGTOFSR3 = L"dlssg_to_fsr3_amd_is_better.dll";

		// check for old NativeHDR being present
		for (auto& moduleName : moduleNamesNativeHDR) {
			if (isModuleLoaded(moduleName)) {
				__REPORT(true, fatal, "An old version of the Native(Auto)HDR plugin is loaded. Please remove it while using Luma. It is a successor to the previous mod.")
				return false;
			}
		}

		// check for Shader Injector being present
		bool bShaderInjectorFound = false;
		for (auto& moduleName : moduleNamesShaderInjector) {
			if (!bShaderInjectorFound && isModuleLoaded(moduleName)) {
				bShaderInjectorFound = true;
			}
		}

		if (!bShaderInjectorFound) {
			__REPORT(true, fatal, "Starfield Shader Injector is not loaded. Luma requires it to function properly. Please download and install the plugin - https://www.nexusmods.com/starfield/mods/5562")
			return false;
		}

		*DisplayMode.value = std::clamp((int32_t)DisplayMode.value.get_data(), 0, 2);  // Clamp to valid range

		const auto previousActualDisplayMode = GetActualDisplayMode();

		// check if Nukem's dlss fg to fsr 3 fg is present
		bIsDLSSFGToFSRFGPresent = isModuleLoaded(moduleNameDLSSGTOFSR3);
		if (bIsDLSSFGToFSRFGPresent && !bIsDLSSFGToFSRFGPatched) {
			bIsDLSSFGToFSRFGPatched = true;
		    Hooks::Patches::PatchStreamline();
		}
		if (*DLSSFGToFSRFGMod.value != bIsDLSSFGToFSRFGPresent) {
			*DLSSFGToFSRFGMod.value = bIsDLSSFGToFSRFGPresent;
			Save();
		}

		// check hdr support
		RefreshHDRDisplaySupportState();

		// enable hdr if off and display mode suggests it should be on (and more)
		RefreshHDRDisplayEnableState();

		// "bIsDLSSFGToFSRFGPresent" and "RefreshHDRDisplayEnableState()" might have changed the "actual" display mode.
		// No need to refresh the swapchain ("RefreshSwapchainFormat()") here as it's not been fully created yet.
		if (previousActualDisplayMode != GetActualDisplayMode()) {
			const RE::BS_DXGI_FORMAT newFormat = GetDisplayModeFormat();
			Utils::SetBufferFormat(RE::Buffers::FrameBuffer, newFormat);

			swapChainObject->format = newFormat;
		}

		return true;
	}

    void Main::RefreshHDRDisplaySupportState()
    {
		bIsHDRSupported = Utils::IsHDRSupported(swapChainObject->hwnd);
		bIsHDREnabled = Utils::IsHDREnabled(swapChainObject->hwnd);
	}

	// This function does a little more than its name might hint at
    void Main::RefreshHDRDisplayEnableState()
    {
		if (bIsHDRSupported && !bIsHDREnabled && IsGameRenderingSetToHDR()) {
		    bIsHDREnabled = Utils::SetHDREnabled(swapChainObject->hwnd);
		}
		
		// change display mode setting if it's hdr and hdr is not supported
		if (!bIsHDRSupported && IsGameRenderingSetToHDR()) {
			*DisplayMode.value = 0;
			// No need to save, the user might have moved the game to an SDR display temporarily
		}
		
		// autodetect peak brightness (only works reliably if HDR is enabled).
		// Note that this value is cached on swapchain creation by DX, so it becomes outdated if the game window was moved to another screen,
		// or if the user toggle HDR or changed their HDR calibration. This means the detection can only really work on startup for now,
		// unless we wrote code to create a new swapchain/factory and get fresh data from DX.
		if (bIsHDREnabled) {
			float detectedMaxLuminance;
			if (Utils::GetHDRMaxLuminance(swapChainObject->swapChainInterface, detectedMaxLuminance)) {
				detectedMaxLuminance = std::max(detectedMaxLuminance, 80.f); // HDR10 min would be 400 nits, but we let it go as low as 80 anyway
				PeakBrightness.defaultValue = detectedMaxLuminance;
				if (PeakBrightnessAutoDetected.get_data() == false) {
					*PeakBrightnessAutoDetected = true;
					*PeakBrightness.value = detectedMaxLuminance;
					Save();
				}
			}
		}
	}

    bool Main::IsSDRForcedOnHDR(bool bAcknowledgeScreenshots) const
    {
		// The game will tonemap to SDR if this is true
		return ForceSDROnHDR.value.get_data() || (bAcknowledgeScreenshots && !bRequestedHDRScreenshot && bRequestedSDRScreenshot && IsDisplayModeSetToHDR());
    }

    bool Main::IsDisplayModeSetToHDR() const
    {
		// Note: this should acknowledge how the display mode is changed by "GetActualDisplayMode()"
		return DisplayMode.value.get_data() > 0;
    }
	
    bool Main::IsGameRenderingSetToHDR(bool bAcknowledgeScreenshots) const
    {
		// The game will tonemap to SDR if this is false
		return IsDisplayModeSetToHDR() && !IsSDRForcedOnHDR(bAcknowledgeScreenshots);
    }

	bool Main::IsFSR3FGEnabled() const
	{
		const RE::FrameGenerationTech frameGenerationTech = *Offsets::uiFrameGenerationTech;
		return frameGenerationTech == RE::FrameGenerationTech::kFSR3;
	}

	bool Main::IsCustomToneMapper() const
	{
		return IsDisplayModeSetToHDR() || ToneMapperType.value.get_data() > 0;
	}

    bool Main::IsFilmGrainTypeImproved() const
	{
		return FilmGrainType.value.get_data() == 1;
	}

    int32_t Main::GetActualDisplayMode(bool bAcknowledgeScreenshots, std::optional<RE::FrameGenerationTech> a_frameGenerationTech) const
	{
		// This mode is for development only and only supports scRGB to it bypasses the display mode branches by frame generation tech
		if (IsSDRForcedOnHDR(bAcknowledgeScreenshots)) {
		    return -1;
		}

		const auto value = std::clamp((int32_t)DisplayMode.value.get_data(), 0, 2); // Clamp to valid range

		RE::FrameGenerationTech frameGenerationTech = a_frameGenerationTech.has_value() ? a_frameGenerationTech.value() : *Offsets::uiFrameGenerationTech;

		if (frameGenerationTech > RE::FrameGenerationTech::kNone && !EnforceUserDisplayMode.value.get_data()) {
			// force scRGB HDR with fsr3 or dlssg if dlssg_to_fsr3 is present
			if (value == 1 && (frameGenerationTech == RE::FrameGenerationTech::kFSR3 || (frameGenerationTech == RE::FrameGenerationTech::kDLSSG && bIsDLSSFGToFSRFGPresent))) {
			    return 2;
			}

			// otherwise force HDR10
			if (value == 2 && frameGenerationTech == RE::FrameGenerationTech::kDLSSG && !bIsDLSSFGToFSRFGPresent) {
			    return 1;
			}
		}

		return value;
	}

    RE::BS_DXGI_FORMAT Main::GetDisplayModeFormat(std::optional<RE::FrameGenerationTech> a_frameGenerationTech) const
    {
		switch (GetActualDisplayMode(false, a_frameGenerationTech)) {
		default:
		case 0:
		case 1:
			return RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R10G10B10A2_UNORM;
		case -1:
		case 2:
			return RE::BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
    }

    DXGI_COLOR_SPACE_TYPE Main::GetDisplayModeColorSpaceType() const
    {
		switch (GetActualDisplayMode()) {
		default:
		case 0:
			return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		case 1:
			return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		case -1:
		case 2:
			return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
		}
    }

    void Main::RefreshSwapchainFormat(std::optional<RE::FrameGenerationTech> a_frameGenerationTech)
	{
		// This should be "safe" being called from all threads
		const RE::BS_DXGI_FORMAT newFormat = GetDisplayModeFormat(a_frameGenerationTech);
		Utils::SetBufferFormat(RE::Buffers::FrameBuffer, newFormat);

		// This should be "safe" being called from all threads
		swapChainObject->format = newFormat;

		// toggle vsync to force a swapchain recreation (it will seemengly happen in one of the renderer threads, even if this is called by the main/game thread)
		Offsets::ToggleVsync(reinterpret_cast<void*>(*Offsets::unkToggleVsyncArg1Ptr + 0x8), *Offsets::bEnableVsync);
	}

    void Main::OnDisplayModeChanged()
	{
		// When we change the display mode we need to refresh some internal FSR 3 stuff (this might already be refreshed when toggling FSR 3, but not when changing other display settings)
		if (*Offsets::uiFrameGenerationTech == RE::FrameGenerationTech::kFSR3) {
			bNeedsToRefreshFSR3 = true;
		}

		RefreshHDRDisplaySupportState();
		RefreshHDRDisplayEnableState();

		RefreshSwapchainFormat();
	}

    void Main::GetShaderConstants(ShaderConstants& a_outShaderConstants, ShaderConstantsMode a_shaderConstantsMode) const
    {
		a_outShaderConstants.DisplayMode = GetActualDisplayMode(true);
		// TODO: expose HDR screenshot normalization as a user (advanced/config only) setting
		if (bRequestedHDRScreenshot) {
			// Unlock HDR range to HDR10 max when taking screenshots, so they appear consistently independently of the user peak brightness.
			// For now we don't force the paper white to 203 (reference/suggested value) because that'd be a bit confusing for users.
			a_outShaderConstants.PeakBrightness = 10000.f;
		}
		else {
			a_outShaderConstants.PeakBrightness = static_cast<float>(PeakBrightness.value.get_data());
		}
		a_outShaderConstants.GamePaperWhite = static_cast<float>(GamePaperWhite.value.get_data());
		a_outShaderConstants.UIPaperWhite = static_cast<float>(UIPaperWhite.value.get_data());
		a_outShaderConstants.ExtendGamut = static_cast<float>(ExtendGamut.value.get_data()) * 0.01f;                      // 0-100 to 0-1
		a_outShaderConstants.bAutoHDRVideos = static_cast<uint32_t>(AutoHDRVideos.value.get_data());
		// There is no reason this wouldn't work in HDR, but for now it's disabled
		a_outShaderConstants.SDRSecondaryBrightness = IsGameRenderingSetToHDR(true) ? 1.f : (static_cast<float>(SecondaryBrightness.value.get_data()) * 0.02f); // 0-100 to 0-2

		a_outShaderConstants.ToneMapperType = static_cast<uint32_t>(ToneMapperType.value.get_data());
		a_outShaderConstants.Saturation = Utils::linearNormalization(static_cast<float>(Saturation.value.get_data()), 0.f, 100.f, 0.5f, 1.5f); // 0-100 to 0.5 - 1.5
		a_outShaderConstants.Contrast = Utils::linearNormalization(static_cast<float>(Contrast.value.get_data()), 0.f, 100.f, 0.5f, 1.5f);     // 0-100 to 0.5 - 1.5
		a_outShaderConstants.Highlights = IsCustomToneMapper()
			? static_cast<float>(Highlights.value.get_data()) * 0.01f // 0-100 to 0-1
			: 0.5f;
		a_outShaderConstants.Shadows = IsCustomToneMapper() 
			? static_cast<float>(Shadows.value.get_data()) * 0.01f    // 0-100 to 0-1
			: 0.5f;
		a_outShaderConstants.Bloom = static_cast<float>(Bloom.value.get_data() * 0.01f);                                    // 0-100 to 0-1

		a_outShaderConstants.ColorGradingStrength = static_cast<float>(ColorGradingStrength.value.get_data()) * 0.01f;    // 0-100 to 0-1
		a_outShaderConstants.LUTCorrectionStrength = static_cast<float>(LUTCorrectionStrength.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.StrictLUTApplication = static_cast<uint32_t>(StrictLUTApplication.value.get_data());

		a_outShaderConstants.GammaCorrectionStrength = static_cast<float>(GammaCorrectionStrength.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.FilmGrainType = static_cast<uint32_t>(FilmGrainType.value.get_data());
		a_outShaderConstants.FilmGrainFPSLimit = static_cast<float>(FilmGrainFPSLimit.value.get_data());
		a_outShaderConstants.PostSharpen = static_cast<uint32_t>(PostSharpen.value.get_data());

		a_outShaderConstants.bIsAtEndOfFrame = static_cast<uint32_t>(bIsAtEndOfFrame.load());
		a_outShaderConstants.RuntimeMS = *Offsets::g_durationOfApplicationRunTimeMS;
		a_outShaderConstants.DevSetting01 = static_cast<float>(DevSetting01.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.DevSetting02 = static_cast<float>(DevSetting02.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.DevSetting03 = static_cast<float>(DevSetting03.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.DevSetting04 = static_cast<float>(DevSetting04.value.get_data()) * 0.01f;  // 0-100 to 0-1
		a_outShaderConstants.DevSetting05 = static_cast<float>(DevSetting05.value.get_data()) * 0.01f;  // 0-100 to 0-1

		if (a_shaderConstantsMode == ShaderConstantsMode::kLUT && VanillaMenuLUTs.value.get_data() && !Utils::ShouldCorrectLUTs()) {
			a_outShaderConstants.LUTCorrectionStrength = 0.f;
			a_outShaderConstants.ColorGradingStrength = 1.f;
		}
    }

    void Main::InitConfig(bool a_bIsSFSE)
	{
		config = a_bIsSFSE ? &sfseConfig : &asiConfig;
	}

    void Main::RegisterReshadeOverlay()
    {
		if (!bReshadeSettingsOverlayRegistered) {
			HMODULE hModule = nullptr;
			GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(Main::GetSingleton()), &hModule);
			if (hModule) {
				if (reshade::register_addon(hModule)) {
					reshade::register_overlay("Starfield Luma Settings", DrawReshadeSettings);
					bReshadeSettingsOverlayRegistered = true;
				}
			}
		}
    }

    void Main::Load() noexcept
	{
		static std::once_flag ConfigInit;
		std::call_once(ConfigInit, [&]() {
			// HDR
			config->Bind(DisplayMode.value, DisplayMode.defaultValue);
			config->Bind(EnforceUserDisplayMode.value, EnforceUserDisplayMode.defaultValue);
			config->Bind(ForceSDROnHDR.value, ForceSDROnHDR.defaultValue);
			config->Bind(PeakBrightness.value, PeakBrightness.defaultValue);
			config->Bind(GamePaperWhite.value, GamePaperWhite.defaultValue);
			config->Bind(UIPaperWhite.value, UIPaperWhite.defaultValue);
			config->Bind(ExtendGamut.value, ExtendGamut.defaultValue);
			config->Bind(AutoHDRVideos.value, AutoHDRVideos.defaultValue);

			// SDR
			config->Bind(SecondaryBrightness.value, SecondaryBrightness.defaultValue);

			// Tone-mapper
			config->Bind(ToneMapperType.value, ToneMapperType.defaultValue);
			config->Bind(Saturation.value, Saturation.defaultValue);
			config->Bind(Contrast.value, Contrast.defaultValue);
			config->Bind(Highlights.value, Highlights.defaultValue);
			config->Bind(Shadows.value, Shadows.defaultValue);
			config->Bind(Bloom.value, Bloom.defaultValue);

			// Color Grading
			config->Bind(ColorGradingStrength.value, ColorGradingStrength.defaultValue);
			config->Bind(LUTCorrectionStrength.value, LUTCorrectionStrength.defaultValue);
			config->Bind(VanillaMenuLUTs.value, VanillaMenuLUTs.defaultValue);
			config->Bind(StrictLUTApplication.value, StrictLUTApplication.defaultValue);

			config->Bind(GammaCorrectionStrength.value, GammaCorrectionStrength.defaultValue);
			config->Bind(FilmGrainType.value, FilmGrainType.defaultValue);
			config->Bind(FilmGrainFPSLimit.value, FilmGrainFPSLimit.defaultValue);
			config->Bind(PostSharpen.value, PostSharpen.defaultValue);
			config->Bind(HDRScreenshots.value, HDRScreenshots.defaultValue);
			config->Bind(HDRScreenshotsLossless.value, HDRScreenshotsLossless.defaultValue);
			config->Bind(DLSSFGToFSRFGMod.value, DLSSFGToFSRFGMod.defaultValue);
			config->Bind(DevSetting01.value, DevSetting01.defaultValue);
			config->Bind(DevSetting02.value, DevSetting02.defaultValue);
			config->Bind(DevSetting03.value, DevSetting03.defaultValue);
			config->Bind(DevSetting04.value, DevSetting04.defaultValue);
			config->Bind(DevSetting05.value, DevSetting05.defaultValue);
			config->Bind(RenderTargetsToUpgrade,
				"ImageSpaceBuffer",
				"ScaleformCompositeBuffer", // Not upgrading this could cause issues with FSR FG as the swapchain would be in a different format than the UI buffer (untested), and maybe it would break AutoHDR bink videos.
				"SF_ColorBuffer",
				"HDRImagespaceBuffer",
				"ImageSpaceHalfResBuffer",
				"ImageProcessColorTarget",
				"ImageSpaceBufferB10G11R11",
				"ImageSpaceBufferE5B9G9R9",
				"TAA_idTech7HistoryColorTarget",
				"EnvBRDF",
				"ImageSpaceBufferR10G10B10A2"
			);
			config->Bind(ExtraRenderTargetsToUpgrade,
				"NativeResolutionColorBuffer01",  // issues on AMD
				"ColorBuffer01"  // issues on AMD
			);
			config->Bind(UpgradeExtraRenderTargets, false);
			config->Bind(PeakBrightnessAutoDetected, false);
		});

		config->Load();

		INFO("Config loaded"sv)

		// Default to the last value saved in the config, to avoid issues on startup if we detected the DLSS to FSR FG mod too late,
		// which makes the display mode change in a non thread safe (?) manner that makes the game crash.
		// This workaround avoids crashes all the times except once when adding or removing the mod.
		bIsDLSSFGToFSRFGPresent = *DLSSFGToFSRFGMod.value;
	}

    void Main::Save() noexcept
    {
		std::lock_guard<std::mutex> lg(configMutex);
		config->Generate();
		config->Write();
    }

    void Main::DrawReshadeSettings(reshade::api::effect_runtime*)
    {
        const auto settings = Settings::Main::GetSingleton();
		settings->DrawReshadeSettings();
    }

    void Main::DrawReshadeTooltip(const char* a_desc)
	{
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 8 });
			if (ImGui::BeginTooltip()) {
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
				ImGui::TextUnformatted(a_desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
			ImGui::PopStyleVar();
		}
	}

    bool Main::DrawReshadeCheckbox(Checkbox& a_checkbox)
    {
		bool result = false;
		bool tempValue = *a_checkbox.value;
		if (ImGui::Checkbox(a_checkbox.name.c_str(), &tempValue)) {
			*a_checkbox.value = tempValue;
			Save();
			result = true;
		}
		DrawReshadeTooltip(a_checkbox.description.c_str());
		if (DrawReshadeResetButton(a_checkbox)) {
			*a_checkbox.value = a_checkbox.defaultValue;
			Save();
			result = true;
		}
	    return result;
    }

    bool Main::DrawReshadeEnumStepper(EnumStepper& a_stepper)
    {
		bool result = false;
		int tempValue = *a_stepper.value;
		if (ImGui::SliderInt(a_stepper.name.c_str(), &tempValue, 0, a_stepper.GetNumOptions() - 1, a_stepper.GetStepperText(tempValue).c_str(), ImGuiSliderFlags_NoInput)) {
			*a_stepper.value = tempValue;
			Save();
			result = true;
		}
		DrawReshadeTooltip(a_stepper.description.c_str());
		if (DrawReshadeResetButton(a_stepper)) {
			*a_stepper.value = a_stepper.defaultValue;
			Save();
			result = true;
		}
		return result;
    }

    bool Main::DrawReshadeValueStepper(ValueStepper& a_stepper)
	{
		bool result = false;
		int tempValue = *a_stepper.value;
		if (ImGui::SliderInt(a_stepper.name.c_str(), &tempValue, a_stepper.minValue, a_stepper.maxValue, std::to_string(tempValue).c_str())) {
			*a_stepper.value = tempValue;
			Save();
			result = true;
		}
		DrawReshadeTooltip(a_stepper.description.c_str());
		if (DrawReshadeResetButton(a_stepper)) {
			*a_stepper.value = a_stepper.defaultValue;
			Save();
			result = true;
		}
		return result;
	}

    bool Main::DrawReshadeSlider(Slider& a_slider)
    {
		bool result = false;
		float tempValue = *a_slider.value;
		if (ImGui::SliderFloat(a_slider.name.c_str(), &tempValue, a_slider.sliderMin, a_slider.sliderMax, "%.0f")) {
			*a_slider.value = tempValue;
			Save();
			result = true;
		}
		DrawReshadeTooltip(a_slider.description.c_str());
		if (DrawReshadeResetButton(a_slider)) {
			*a_slider.value = a_slider.defaultValue;
			Save();
			result = true;
		}
		return result;
    }

    bool Main::DrawReshadeResetButton(Setting& a_setting)
	{
		bool bResult = false;
		ImGui::SameLine();
		ImGui::PushID(&a_setting);
		if (!a_setting.IsDefault()) {
			if (ImGui::SmallButton(ICON_FK_UNDO)) {
				bResult = true;
			}
		} else {
			const auto& style = ImGui::GetStyle();
			const float width = ImGui::CalcTextSize(ICON_FK_UNDO).x + style.FramePadding.x * 2.f;
			ImGui::InvisibleButton("", ImVec2(width, 0));
		}
		ImGui::PopID();
		return bResult;
	}

    void Main::DrawReshadeSettings()
    {
		ImGui::SetWindowSize(ImVec2(0, 0));
		const auto& io = ImGui::GetIO();
		const auto currentPos = ImGui::GetWindowPos();
		ImGui::SetWindowPos(ImVec2(io.DisplaySize.x / 3, currentPos.y), ImGuiCond_FirstUseEver);

		const bool isSDRForcedOnHDR = IsSDRForcedOnHDR();
		const bool isGameRenderingSetToHDR = IsGameRenderingSetToHDR();
		const bool isFSR3FGEnabled = IsFSR3FGEnabled();
		const bool isCustomToneMapper = IsCustomToneMapper();
		if (IsHDRSupported()) {
			// TODO: fix, these can often crash when changed during gameplay if FG is enabled (it doesn't seem to be a threading issue).
			// FSR FG crashes all the times while DLSS FG crashes sometimes.
			// For now we work around it by not exposing it.
			bool canChangeDisplayMode = true;
#if !DEVELOPMENT
			canChangeDisplayMode = *Offsets::uiFrameGenerationTech == RE::FrameGenerationTech::kNone;
#endif
			if (canChangeDisplayMode && DrawReshadeEnumStepper(DisplayMode)) {
				OnDisplayModeChanged();
			}
#if DEVELOPMENT
			if (DrawReshadeCheckbox(ForceSDROnHDR)) {
				OnDisplayModeChanged();
			}
			if (DrawReshadeCheckbox(EnforceUserDisplayMode)) {
				OnDisplayModeChanged();
			}
#endif
		}

		if (isGameRenderingSetToHDR) {
			DrawReshadeValueStepper(PeakBrightness);
			DrawReshadeValueStepper(GamePaperWhite);
#if !FSR_3_FG_SUPPORTS_UI_PAPER_WHITE
			if (!isFSR3FGEnabled)
#endif
			{
				DrawReshadeValueStepper(UIPaperWhite);
			}
		}
		else
		{
			if (isSDRForcedOnHDR) {
				DrawReshadeValueStepper(GamePaperWhite);
			}
			DrawReshadeSlider(SecondaryBrightness);
		}

		DrawReshadeEnumStepper(ToneMapperType);
		DrawReshadeSlider(Saturation);
		DrawReshadeSlider(Contrast);
		if (isCustomToneMapper) {
			DrawReshadeSlider(Highlights);
			DrawReshadeSlider(Shadows);
		}
		if (isGameRenderingSetToHDR) {
			DrawReshadeSlider(ExtendGamut);
		}
		DrawReshadeSlider(Bloom);

		DrawReshadeSlider(ColorGradingStrength);
		DrawReshadeSlider(LUTCorrectionStrength);
		DrawReshadeCheckbox(VanillaMenuLUTs);
		if (isGameRenderingSetToHDR) {
			DrawReshadeCheckbox(StrictLUTApplication);
			DrawReshadeCheckbox(AutoHDRVideos);
		}

		DrawReshadeSlider(GammaCorrectionStrength);
		DrawReshadeEnumStepper(FilmGrainType);
		if (IsFilmGrainTypeImproved()) {
			DrawReshadeSlider(FilmGrainFPSLimit);
		}
		DrawReshadeCheckbox(PostSharpen);
		ImGui::Spacing();
		DrawReshadeCheckbox(HDRScreenshots);
		if (HDRScreenshots.value) {
			ImGui::SameLine();
			DrawReshadeCheckbox(HDRScreenshotsLossless);
		}

#if DEVELOPMENT
		DrawReshadeSlider(DevSetting01);
		DrawReshadeSlider(DevSetting02);
		DrawReshadeSlider(DevSetting03);
		DrawReshadeSlider(DevSetting04);
		DrawReshadeSlider(DevSetting05);
#endif
    }
}
