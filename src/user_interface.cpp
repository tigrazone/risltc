//  Copyright (C) 2021, Christoph Peters, Karlsruhe Institute of Technology
//  Copyright (C) 2023, Ishaan Shah, International Institute of Information Technology, Hyderabad
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "user_interface.h"
#include "string_utilities.h"
#include "frame_timer.h"
#include "math_utilities.h"
#include <cstring>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <iostream>

void specify_user_interface(application_updates_t* updates, application_t* app, float frame_time, uint32_t* reset_accum) {
	// A few preparations
	ImGui::SetCurrentContext((ImGuiContext*) app->imgui.handle);
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Scene and render settings");
	scene_specification_t* scene = &app->scene_specification;
	render_settings_t* settings = &app->render_settings;

	
	// Display some help text
	ImGui::Text("[?]");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(
			"LMB             Rotate camera\n"
			"WASDRF, arrows  Move camera\n"
			"IKJL            Rotate camera\n"
			",.              FOV camera\n"
			"Ctrl            Move slower\n"
			"Shift           Move faster\n"
			"F1              Toggle user interface\n"
			"F2              Toggle v-sync\n"
			"F3              Quick save (camera and lights)\n"
			"F4              Quick load (camera and lights)\n"
			"F5              Reload shaders\n"
			"F10, F12        Take screenshot"
		);
	// Display the frame rate
	ImGui::SameLine();
	ImGui::Text("Frame %d time: %.2f ms", app->accum_num, frame_time * 1000.0f);
	// Display a text that changes each frame to indicate to the user whether
	// the renderer is running

	static uint32_t frame_index = 0;
	++frame_index;
	ImGui::SameLine();
	const char* progress_texts[] = {" ......", ". .....", ".. ....", "... ...", ".... ..", "..... .", "...... "};
	ImGui::Text(progress_texts[frame_index % COUNT_OF(progress_texts)]);
	

	// Scene selection
	int scene_index = 0;
	const uint32_t sz_g_scene_paths = COUNT_OF(g_scene_paths);

	for (; scene_index != sz_g_scene_paths; ++scene_index) {
		int offset = (int) strlen(scene->file_path) - (int) strlen(g_scene_paths[scene_index][1]);
		if (offset >= 0 && strcmp(scene->file_path + offset, g_scene_paths[scene_index][1]) == 0)
			break;
	}
	const char* scene_names[sz_g_scene_paths];

	for (uint32_t i = 0; i != sz_g_scene_paths; ++i)
		scene_names[i] = g_scene_paths[i][0];
	if (ImGui::Combo("Scene", &scene_index, scene_names, COUNT_OF(scene_names))) {
		free(scene->file_path);
		free(scene->quick_save_path);
		free(scene->texture_path);
		scene->file_path = copy_string(g_scene_paths[scene_index][1]);
		scene->texture_path = copy_string(g_scene_paths[scene_index][2]);
		scene->quick_save_path = copy_string(g_scene_paths[scene_index][3]);
		updates->quick_load = updates->reload_scene = VK_TRUE;
	}

	//tigra: unneeded settings
	/*
	const char* polygon_sampling_techniques[sample_polygon_count];
	polygon_sampling_techniques[sample_polygon_baseline] = "Baseline (zero cost, bogus results)";
	polygon_sampling_techniques[sample_polygon_area_turk] = "Uniform Area Sampling (Turk)";
	polygon_sampling_techniques[sample_polygon_projected_solid_angle] = "Projected Solid Angle Sampling (Peters)";
	polygon_sampling_techniques[sample_polygon_projected_solid_angle_biased] = "Biased Projected Solid Angle Sampling (Peters)";
	polygon_sampling_techniques[sample_polygon_ltc_cp] = "LTC (Ours)";
	if (ImGui::Combo("Polygon sampling", (int*) &settings->polygon_sampling_technique,  polygon_sampling_techniques, sample_polygon_count))
		updates->change_shading = VK_TRUE;
	*/

	/*
	// Sampling settings
	bool show_mis = settings->polygon_sampling_technique == sample_polygon_projected_solid_angle || settings->polygon_sampling_technique == sample_polygon_projected_solid_angle_biased;
	if (show_mis) {
		const char* mis_heuristics[mis_heuristic_count];
		mis_heuristics[mis_heuristic_balance] = "Balance (Veach)";
		mis_heuristics[mis_heuristic_power] = "Power (exponent 2, Veach)";
		mis_heuristics[mis_heuristic_weighted] = "Weighted balance heuristic (Peters)";
		mis_heuristics[mis_heuristic_optimal_clamped] = "Clamped optimal heuristic (Peters)";
		mis_heuristics[mis_heuristic_optimal] = "Optimal heuristic (Peters)";
		uint32_t mis_heuristic_count = COUNT_OF(mis_heuristics);
		if (ImGui::Combo("MIS heuristic", (int*) &settings->mis_heuristic, mis_heuristics, mis_heuristic_count))
			updates->change_shading = VK_TRUE;
	}
	if (show_mis && (settings->mis_heuristic == mis_heuristic_optimal_clamped || settings->mis_heuristic == mis_heuristic_optimal))
		if(ImGui::DragFloat("MIS visibility estimate", &settings->mis_visibility_estimate, 0.01f, 0.0f, 1.0f, "%.2f")) *reset_accum = 1;
	*/

	{
		// Light sampling strategy
		const char* light_sampling_strategies[3];
		light_sampling_strategies[light_uniform] = "Uniform";
		light_sampling_strategies[light_reservoir] = "RIS";
		// Create the interface and remap outputs
		if (ImGui::Combo("Light sampling", (int *) &settings->light_sampling, light_sampling_strategies, 2))
			updates->change_shading = VK_TRUE;
	}

	// Switching vertical synchronization
	if (ImGui::Checkbox("Vsync", (bool*) &settings->v_sync))
		updates->recreate_swapchain = VK_TRUE;
	// Use framebuffer accumulation
	if (ImGui::Checkbox("Accumlation", (bool*) &settings->accum))
		*reset_accum = 1;
	// Changing the sample count
	if (ImGui::InputInt("Sample count", (int*) &settings->sample_count, 1, 10)) {
		if (settings->sample_count < 1) settings->sample_count = 1;
		updates->change_shading = VK_TRUE;
	}
	if (ImGui::InputInt("Sample count light", (int*) &settings->sample_count_light, 1, 10)) {
		if (settings->sample_count_light < 1) settings->sample_count_light = 1;
		updates->change_shading = VK_TRUE;
	}

	// Source of pseudorandom numbers
	const char* noise_types[noise_type_full_count];
	noise_types[noise_type_white] = "White noise";
	noise_types[noise_type_blue] = "Blue noise (1D)";
	noise_types[noise_type_sobol] = "Sobol (2+2D)";
	noise_types[noise_type_owen] = "Owen-scrambled Sobol (2+2D)";
	noise_types[noise_type_burley_owen] = "Burley's Owen-scrambled Sobol (2+2D)";
	noise_types[noise_type_ahmed] = "Ahmed's blue-noise diffusion for Sobol (2+2D)";
	noise_types[noise_type_blue_noise_dithered] = "Blue noise dithered (2D)";
	if (ImGui::Combo("Noise type", (int*) &settings->noise_type, noise_types, noise_type_count))
		updates->regenerate_noise = VK_TRUE;
	ImGui::Checkbox("Animate noise", (bool*) &settings->animate_noise);


	// Various rendering settings
	if(ImGui::DragFloat("Exposure", &settings->exposure_factor, 0.05f, 0.0f, 200.0f, "%.2f")) *reset_accum = 1;

	if(ImGui::DragFloat("Roughness factor", &settings->roughness_factor, 0.01f, 0.0f, 2.0f, "%.2f")) *reset_accum = 1;
	// Polygonal light controls
	if (ImGui::Checkbox("Show polygonal lights", (bool*) &settings->show_polygonal_lights))
		updates->change_shading = VK_TRUE;

	for (uint32_t i = 0; i < scene->polygonal_light_count; ++i) {
		char* group_name = format_uint("Polygonal light %u", i);
		polygonal_light_t* light = &scene->polygonal_lights[i];
		if (ImGui::TreeNode(group_name)) {
			float angles_degrees[3] = { light->rotation_angles[0] * M_180_DIV_PI, light->rotation_angles[1] * M_180_DIV_PI, light->rotation_angles[2] * M_180_DIV_PI};
			ImGui::DragFloat3("Rotation (Euler angles)", angles_degrees, 0.1f, -180.0f, 180.0f);
			for (uint32_t i = 0; i != 3; ++i)
				light->rotation_angles[i] = angles_degrees[i] * M_PI_DIV_180;
			float scalings[2] = { light->scaling_x, light->scaling_y };
			ImGui::DragFloat2("Scaling (xy)", scalings, 0.01f, 0.01f, 100.0f);
			light->scaling_x = scalings[0];
			light->scaling_y = scalings[1];
			ImGui::DragFloat3("Translation (xyz)", light->translation, 0.01f);
			for (uint32_t i = 0; i != light->vertex_count; ++i) {
				char* label = format_uint("Vertex %u", i);
				ImGui::DragFloat2(label, &light->vertices_plane_space[i * 4], 0.01f);
				free(label);
			}
			ImGui::ColorEdit3("Radiant flux", light->radiant_flux);
			char texture_path[2048] = "";
			if (light->texture_file_path) strcpy(texture_path, light->texture_file_path);
			ImGui::InputText("Texture path (*.vkt)", texture_path, sizeof(texture_path));
			if ((light->texture_file_path == NULL || std::strcmp(texture_path, light->texture_file_path) != 0)
				&&  (std::strlen(texture_path) == 0
				||  std::strlen(texture_path) > 4 && std::strcmp(".vkt", texture_path + strlen(texture_path) - 4) == 0))
			{
				updates->update_light_textures = VK_TRUE;
				free(light->texture_file_path);
				light->texture_file_path = copy_string(texture_path);
			}
			const char* polygon_texturing_techniques[polygon_texturing_count];
			polygon_texturing_techniques[polygon_texturing_none] = "Disabled";
			polygon_texturing_techniques[polygon_texturing_area] = "Texture";
			polygon_texturing_techniques[polygon_texturing_portal] = "Light probe";
			polygon_texturing_techniques[polygon_texturing_ies_profile] = "IES profile";
			ImGui::Combo("Texture type", (int*) &light->texturing_technique, polygon_texturing_techniques, COUNT_OF(polygon_texturing_techniques));
			if (ImGui::Button("Add vertex")) {
				set_polygonal_light_vertex_count(light, light->vertex_count + 1);
				float* vertices = light->vertices_plane_space;
				size_t lvc4 = (light->vertex_count - 1) << 2;
				vertices[lvc4] = 0.5f * (vertices[0] + vertices[lvc4 - 4]);
				vertices[lvc4 + 1] = 0.5f * (vertices[1] + vertices[lvc4 - 4 + 1]);
				updates->update_light_count = VK_TRUE;
			}
			if (light->vertex_count > 3) {
				ImGui::SameLine();
				if (ImGui::Button("Delete vertex")) {
					set_polygonal_light_vertex_count(light, light->vertex_count - 1);
					updates->update_light_count = VK_TRUE;
				}
			}
			if (scene->polygonal_light_count > 0) {
				ImGui::SameLine();
				if (ImGui::Button("Delete light"))
					light->vertex_count = 0;
			}
			ImGui::TreePop();
		}
		free(group_name);
	}
	// Go over all lights to see which of them have been deleted
	uint32_t new_light_index = 0;
	for (uint32_t i = 0; i < scene->polygonal_light_count; ++i) {
		if (scene->polygonal_lights[i].vertex_count > 0) {
			scene->polygonal_lights[new_light_index] = scene->polygonal_lights[i];
			++new_light_index;
		}
		else {
			destroy_polygonal_light(&scene->polygonal_lights[i]);
			updates->update_light_count = VK_TRUE;
		}
	}
	scene->polygonal_light_count = new_light_index;
	if (ImGui::Button("Add polygonal light")) {
		// Copy over old polygonal lights
		scene_specification_t old = *scene;
		scene->polygonal_lights = (polygonal_light_t*) malloc(sizeof(polygonal_light_t) * (scene->polygonal_light_count + 1));
		memcpy(scene->polygonal_lights, old.polygonal_lights, sizeof(polygonal_light_t) * scene->polygonal_light_count);
		free(old.polygonal_lights);
		// Create a new one
		polygonal_light_t default_light;
		create_default_polygonal_light(&default_light);

		scene->polygonal_lights[scene->polygonal_light_count] = default_light;
		scene->polygonal_light_count = old.polygonal_light_count + 1;
		updates->update_light_count = VK_TRUE;
	}

	// Show buttons for quick save and quick load
	if (ImGui::Button("Quick save"))
		updates->quick_save = VK_TRUE;
	ImGui::SameLine();
	if (ImGui::Button("Quick load"))
		updates->quick_load = VK_TRUE;

	/*
	// A button to reproduce experiments from the publication
	if (ImGui::Button("Reproduce experiments"))
		app->experiment_list.next = 0;
	*/

	// That's all
	ImGui::End();
	ImGui::EndFrame();
}
