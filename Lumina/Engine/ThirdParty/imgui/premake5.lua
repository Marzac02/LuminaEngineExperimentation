project "ImGui"
	kind "StaticLib"
	warnings "off"
	enableunitybuild "Off"


	includedirs
	{
		".",
		"backends",
		LuminaConfig.ThirdPartyDirectory(),
		LuminaConfig.ThirdPartyPath("glfw/include/")
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
		"ImGuizmo.cpp",
        "**.lua",
    }
