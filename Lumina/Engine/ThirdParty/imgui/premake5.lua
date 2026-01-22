
VULKAN_SDK = os.getenv("VULKAN_SDK")

project "ImGui"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")

	includedirs
	{
		".",
		"backends",
		"%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/glfw/include/",
		"%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/",
		"%{VULKAN_SDK}/Include/",
	}

	files
	{
		"imconfig.h",
		"imgui.h",
		"implot.h",
        "implot.cpp",
        "implot_internal.h",
        "implot_items.cpp",
		"imgui.cpp",
		"imgui_draw.cpp",
		"imgui_internal.h",
		"imgui_widgets.cpp",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"imgui_demo.cpp",
        "implot_demo.cpp",
		"backends/imgui_impl_glfw.h",
		"backends/imgui_impl_glfw.cpp",
		"backends/imgui_impl_vulkan.h",
		"backends/imgui_impl_vulkan.cpp",
        "imgui_tables.cpp",
		"ImGuizmo.h",
		"ImGuizmo.cpp"
	}