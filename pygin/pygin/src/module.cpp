﻿/*
module.cpp

*/
/*
Copyright 2017 Alex Alabuzhev
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"

#include "module.hpp"

#include "py.boolean.hpp"
#include "py.bytes.hpp"
#include "py.common.hpp"
#include "py.floating.hpp"
#include "py.integer.hpp"
#include "py.list.hpp"
#include "py.string.hpp"
#include "py.tuple.hpp"
#include "py.uuid.hpp"

#include "far_api.hpp"
#include "error_handling.hpp"
#include "helpers.hpp"

using namespace py::literals;

auto EmptyToNull(const wchar_t* Str)
{
	return Str && *Str? Str : nullptr;
}

module::module(const py::object& Object):
	m_PluginModule(Object),
	m_PluginModuleClass(m_PluginModule["FarPluginClass"])
{
}

bool module::check_function(const wchar_t* FunctionName) const
{
	// Mapped to class static fields
	if (!wcscmp(FunctionName, L"GetGlobalInfoW"))
		return true;

	// Mapped to class ctor
	if (!wcscmp(FunctionName, L"SetStartupInfoW"))
		return true;

	// The rest is as is

	if (!m_PluginModuleClass.has_attribute(FunctionName))
		return false;

	const auto Function = m_PluginModuleClass[FunctionName];
	if (!py::callable_check(Function))
		return false;

	m_PluginModuleClassFunctions.emplace(FunctionName, Function);
	return true;
}

template<typename ... args>
py::object module::call(const wchar_t* FunctionName, const args&... Args) const
{
	if (!m_PluginModuleInstance)
		throw silent_exception{};

	return m_PluginModuleClassFunctions.at(FunctionName)(m_PluginModuleInstance, Args...);
}

HANDLE module::AnalyseW(const AnalyseInfo* Info)
{
	return nullptr;
}

void module::CloseAnalyseW(const CloseAnalyseInfo* Info)
{
}

void module::ClosePanelW(const ClosePanelInfo* Info)
{
}

intptr_t module::CompareW(const CompareInfo* Info)
{
	return 0;
}

intptr_t module::ConfigureW(const ConfigureInfo* Info)
{
	auto ConfigureInstance = far_api::type("ConfigureInfo"s)();

	ConfigureInstance["Guid"] = *Info->Guid;

	return py::cast<bool>(call(L"ConfigureW", ConfigureInstance));
}

intptr_t module::DeleteFilesW(const DeleteFilesInfo* Info)
{
	return 0;
}

void module::ExitFARW(const ExitInfo* Info)
{
	const auto ExitInfoInstance = far_api::type("ExitInfo"s)();
	call(L"ExitFARW", ExitInfoInstance);

	// Point of no return
	m_PluginModuleInstance = {};
	m_PluginModuleClassFunctions.clear();
	m_PluginModuleClass = {};
	m_PluginModule = {};
}

void module::FreeFindDataW(const FreeFindDataInfo* Info)
{
}

intptr_t module::GetFilesW(GetFilesInfo* Info)
{
	return 0;
}

intptr_t module::GetFindDataW(GetFindDataInfo* Info)
{
	return 0;
}

void module::GetGlobalInfoW(GlobalInfo* Info)
{
	Info->Title = (m_Title = py::cast<std::wstring>(m_PluginModuleClass["Title"])).c_str();
	Info->Author = (m_Author = py::cast<std::wstring>(m_PluginModuleClass["Author"])).c_str();
	Info->Description = (m_Description = py::cast<std::wstring>(m_PluginModuleClass["Description"])).c_str();
	Info->Guid = py::cast<UUID>(m_PluginModuleClass["Guid"]);

	const auto Version = m_PluginModuleClass.get_attribute("Version");

	// This won't work as far_api is not initialised yet
	// Consider 2-stage initialisation
	//Version.ensure_type(far_api::type("VersionInfo"s));

	Info->Version.Major = py::cast<DWORD>(Version["Major"]);
	Info->Version.Minor = py::cast<DWORD>(Version["Minor"]);
	Info->Version.Revision = py::cast<DWORD>(Version["Revision"]);
	Info->Version.Build = py::cast<DWORD>(Version["Build"]);
	Info->Version.Stage = py::cast<VERSION_STAGE>(Version["Stage"]);
}

void module::GetOpenPanelInfoW(OpenPanelInfo* Info)
{
}

void module::GetPluginInfoW(PluginInfo* Info)
{
	auto PluginInfoType = far_api::type("PluginInfo"s);
	const auto PyInfo = call(L"GetPluginInfoW");
	PyInfo.ensure_type(PluginInfoType);

	Info->Flags = py::cast<unsigned long long>(PyInfo["Flags"]);

	const auto& ConvertPluginMenuItem = [&](const char* Kind, menu_items& MenuItems, PluginMenuItem& Destination)
	{
		const auto ItemsList = py::cast<py::list>(PyInfo[Kind]);

		const auto ListSize = ItemsList.size();

		const auto prepare = [ListSize](auto& Container)
		{
			Container.clear();
			Container.reserve(ListSize);
		};

		prepare(MenuItems.StringsData);
		prepare(MenuItems.Strings);
		prepare(MenuItems.Uuids);

		// TODO: enumerator
		for (size_t i = 0; i != ListSize; ++i)
		{
			const auto Tuple = py::cast<py::tuple>(ItemsList[i]);
			MenuItems.StringsData.emplace_back(py::cast<std::wstring>(Tuple[0]));
			MenuItems.Strings.emplace_back(MenuItems.StringsData.back().c_str());
			MenuItems.Uuids.emplace_back(py::cast<UUID>(Tuple[1]));
		}

		Destination.Strings = MenuItems.Strings.data();
		Destination.Guids = MenuItems.Uuids.data();
		Destination.Count = ListSize;
	};

	ConvertPluginMenuItem("PluginMenuItems", m_PluginMenuItems, Info->PluginMenu);
	ConvertPluginMenuItem("DiskMenuItems", m_DiskMenuItems, Info->DiskMenu);
	ConvertPluginMenuItem("PluginConfigItems", m_PluginConfigItems, Info->PluginConfig);

	m_CommandPrefix = py::cast<std::wstring>(PyInfo["CommandPrefix"]);
	Info->CommandPrefix = m_CommandPrefix.c_str();
}

intptr_t module::MakeDirectoryW(MakeDirectoryInfo* Info)
{
	return 0;
}

static py::object ConvertValue(const FarMacroValue& Value)
{
	auto FarMacroValueInstance = far_api::type("FarMacroValue")();

	const auto& Convert = [&]() -> py::object
	{
		switch (Value.Type)
		{
		case FMVT_UNKNOWN:
			return 0_py;

		case FMVT_INTEGER:
			return py::integer(Value.Integer);

		case FMVT_STRING:
			return py::string(Value.String);

		case FMVT_DOUBLE:
			return py::floating(Value.Double);

		case FMVT_BOOLEAN:
			return py::boolean(Value.Boolean != 0);

		case FMVT_BINARY:
			return py::bytes(Value.Binary.Data, Value.Binary.Size);

		case FMVT_POINTER:
			return py::integer(Value.Pointer);

		case FMVT_NIL:
			return {};

		case FMVT_ARRAY:
			return helpers::list::from_array(Value.Array.Values, Value.Array.Count, ConvertValue);

		case FMVT_PANEL:
			return py::integer(Value.Pointer);

		default:
			return {};
		}
	};

	FarMacroValueInstance["Type"] = far_api::type("FarMacroVarType")(Value.Type);
	FarMacroValueInstance["Value"] = Convert();
	return FarMacroValueInstance;
}

HANDLE module::OpenW(const OpenInfo* Info)
{
	auto OpenInfoInstance = far_api::type("OpenInfo"s)();

	OpenInfoInstance["OpenFrom"] = far_api::type("OpenFrom"s)(Info->OpenFrom);
	OpenInfoInstance["Guid"] = *Info->Guid;

	switch(Info->OpenFrom)
	{
	case OPEN_SHORTCUT:
		{
			const auto Data = reinterpret_cast<const OpenShortcutInfo*>(Info->Data);
			auto OpenShortcutInfoInstance = far_api::type("OpenShortcutInfo")();
			OpenShortcutInfoInstance["HostFile"] = Data->HostFile;
			OpenShortcutInfoInstance["ShortcutData"] = Data->ShortcutData;
			OpenShortcutInfoInstance["Flags"] = far_api::type("OpenShortcutFlags")(Data->Flags);
			OpenInfoInstance["Data"] = OpenShortcutInfoInstance;
		}
		break;

	case OPEN_COMMANDLINE:
		{
			const auto Data = reinterpret_cast<const OpenCommandLineInfo*>(Info->Data);
			auto OpenCommandLineInfoInstance = far_api::type("OpenCommandLineInfo")();
			OpenCommandLineInfoInstance["CommandLine"] = Data->CommandLine;
			OpenInfoInstance["Data"] = OpenCommandLineInfoInstance;
		}
		break;

	case OPEN_DIALOG:
		{
			const auto Data = reinterpret_cast<const OpenDlgPluginData*>(Info->Data);
			auto OpenDlgPluginDataInstance = far_api::type("OpenDlgPluginData")();
			OpenDlgPluginDataInstance["Dialog"] = py::integer(Data->hDlg);
			OpenInfoInstance["Data"] = OpenDlgPluginDataInstance;
		}
		break;

	case OPEN_ANALYSE:
		{
			const auto Data = reinterpret_cast<const AnalyseInfo*>(Info->Data);
			auto AnalyseInfoInstance = far_api::type("AnalyseInfo")();
			AnalyseInfoInstance["FileName"] = Data->FileName;
			AnalyseInfoInstance["Buffer"] = py::bytes(Data->Buffer, Data->BufferSize);
			AnalyseInfoInstance["OpMode"] = far_api::type("OperationModes")(Data->OpMode);
			OpenInfoInstance["Data"] = AnalyseInfoInstance;
		}
		break;

	case OPEN_FROMMACRO:
		{
			const auto Data = reinterpret_cast<const OpenMacroInfo*>(Info->Data);
			auto OpenMacroInfoInstance = far_api::type("OpenMacroInfo")();
			OpenMacroInfoInstance["Values"] = helpers::list::from_array(Data->Values, Data->Count, ConvertValue);
			OpenInfoInstance["Data"] = OpenMacroInfoInstance;
		}
		break;

	default:
		break;
	}

	const auto Result = call(L"OpenW", OpenInfoInstance);
	return Result? py::cast<HANDLE>(Result) : nullptr;
}

intptr_t module::ProcessDialogEventW(const ProcessDialogEventInfo* Info)
{
	return 0;
}

intptr_t module::ProcessEditorEventW(const ProcessEditorEventInfo* Info)
{
	return 0;
}

intptr_t module::ProcessEditorInputW(const ProcessEditorInputInfo* Info)
{
	return 0;
}

intptr_t module::ProcessPanelEventW(const ProcessPanelEventInfo* Info)
{
	return 0;
}

intptr_t module::ProcessHostFileW(const ProcessHostFileInfo* Info)
{
	return 0;
}

intptr_t module::ProcessPanelInputW(const ProcessPanelInputInfo* Info)
{
	return 0;
}

intptr_t module::ProcessConsoleInputW(ProcessConsoleInputInfo* Info)
{
	return 0;
}

intptr_t module::ProcessSynchroEventW(const ProcessSynchroEventInfo* Info)
{
	return 0;
}

intptr_t module::ProcessViewerEventW(const ProcessViewerEventInfo* Info)
{
	return 0;
}

intptr_t module::PutFilesW(const PutFilesInfo* Info)
{
	return 0;
}

intptr_t module::SetDirectoryW(const SetDirectoryInfo* Info)
{
	return 0;
}

intptr_t module::SetFindListW(const SetFindListInfo* Info)
{
	return 0;
}

void module::SetStartupInfoW(const PluginStartupInfo* Info)
{
	far_api::initialise(Info);
	m_PluginModuleInstance = m_PluginModuleClass();
}

intptr_t module::GetContentFieldsW(const GetContentFieldsInfo* Info)
{
	return 0;
}

intptr_t module::GetContentDataW(GetContentDataInfo* Info)
{
	return 0;
}

void module::FreeContentDataW(const GetContentDataInfo* Info)
{
}
