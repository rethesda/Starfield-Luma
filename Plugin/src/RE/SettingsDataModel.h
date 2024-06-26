#pragma once

#include "UIDataShuttle.h"
#include "UI.h"

namespace RE
{
	class SubSettingsList
	{
	public:
		class GeneralSetting
		{
		public:
			struct Type
			{
				enum
				{
					Slider = 0,
					Stepper = 1,
					LargeStepper = 2,
					Checkbox = 3,
				};
			};

			struct Category
			{
				enum
				{
					Gameplay = 0,
					Display = 1,
					Interface = 2,
					Controls = 3,
					ControlMappings = 4,
					Audio = 5,
					Accessibility = 6,
				};
			};

			struct SliderData
			{
				TUIValue<float> m_Value;       // 0
				StringUIValue m_DisplayValue;  // 20
				char _pad40[0x8];              // 40
			};

			struct StepperData
			{
				TUIValue<unsigned int> m_Value;                              // 0
				ArrayUIValue<TUIValue<BSFixedStringCS>, 0> m_DisplayValues;  // 20
				char _pad98[0x8];                                            // 98
			};

			struct CheckBoxData
			{
				TUIValue<bool> m_Value;       // 0
				char           _pad20[0x48];  // 20
			};

			struct PEOData
			{
				char _unk[0xF8];  // 0
			};

			StringUIValue m_Text;                        // 00
			StringUIValue m_Description;                 // 20
			StringUIValue m_Preview;				     // 40
			TUIValue<unsigned int> m_ID;                 // 60
			TUIValue<unsigned int> m_Type;               // 80
			TUIValue<unsigned int> m_Category;           // A0
			TUIValue<bool> m_Enabled;                    // C0
			TUIValue<bool> m_Unk;						 // E0
			NestedUIValue<SliderData> m_SliderData;      // 100
			NestedUIValue<StepperData> m_StepperData;    // 198
			NestedUIValue<CheckBoxData> m_CheckBoxData;  // 288
			NestedUIValue<PEOData> m_PEOData;            // 340
			char _unk488[0x10];							 // 488

			GeneralSetting()
			{
				auto addr = dku::Hook::IDToAbs(135831);
				auto func = reinterpret_cast<void (*)(GeneralSetting&)>(addr);
				func(*this);
			}

			~GeneralSetting()
			{
				auto addr = dku::Hook::IDToAbs(135898);
				auto func = reinterpret_cast<void (*)(GeneralSetting&)>(addr);
				func(*this);
			}
		};

		ArrayNestedUIValue<GeneralSetting, 0> m_Settings;  // 0
		char _pad78[0x8];                                  // 78
	};

	class SettingsDataModel
	{
	public:
		class UpdateEventData
		{
		public:
			SettingsDataModel* m_Model;
			union
			{
				bool Bool;
				int Int;
				float Float;
			} m_Value;
			int m_SettingID;
		};

		char _pad0[0x1B8];                                             // 0
		TUIDataShuttleContainerMap<SubSettingsList> m_SubSettingsMap;  // 1B8

		SubSettingsList::GeneralSetting* FindSettingById(int Id)
		{
			// This is supposed to be held under a lock but I don't care enough right now
			for (auto& item : m_SubSettingsMap.GetData().m_Settings.Items())
			{
				if (item.m_ShuttleMap.GetData().m_ID.m_Value == Id)
					return &item.m_ShuttleMap.GetData();
			}

			return nullptr;
		}
	};

	// Sanity checks
	static_assert(sizeof(NestedUIValue<SubSettingsList::GeneralSetting::SliderData>) == 0x98);
	static_assert(sizeof(NestedUIValue<SubSettingsList::GeneralSetting::StepperData>) == 0xF0);
	static_assert(sizeof(NestedUIValue<SubSettingsList::GeneralSetting::CheckBoxData>) == 0xB8);
	static_assert(sizeof(NestedUIValue<SubSettingsList::GeneralSetting::PEOData>) == 0x148);

	static_assert(sizeof(TUIDataShuttleContainerMap<SubSettingsList::GeneralSetting::SliderData>) == 0x70);
	static_assert(sizeof(TUIDataShuttleContainerMap<SubSettingsList::GeneralSetting::StepperData>) == 0xC8);
	static_assert(sizeof(TUIDataShuttleContainerMap<SubSettingsList::GeneralSetting::CheckBoxData>) == 0x90);
	static_assert(sizeof(TUIDataShuttleContainerMap<SubSettingsList::GeneralSetting::PEOData>) == 0x120);

	static_assert(sizeof(ArrayUIValue<NestedUIValue<SubSettingsList::GeneralSetting::SliderData>, 0>) == 0x78);

	static_assert(sizeof(NestedUIValue<SubSettingsList::GeneralSetting>) == 0x4E8);
	static_assert(sizeof(ArrayNestedUIValue<SubSettingsList::GeneralSetting, 0>) == 0x78);

	static_assert(sizeof(TUIDataShuttleContainerMap<SubSettingsList>) == 0xA8);
}
