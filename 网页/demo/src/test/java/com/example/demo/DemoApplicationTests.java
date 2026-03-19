package com.example.demo;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.AutoConfigureMockMvc;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.test.web.servlet.MockMvc;

import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@SpringBootTest
@AutoConfigureMockMvc
class DemoApplicationTests {

	@Autowired
	private MockMvc mockMvc;

	@Test
	void contextLoads() {
	}

	@Test
	void healthEndpointWorks() throws Exception {
		mockMvc.perform(get("/api/health"))
				.andExpect(status().isOk())
				.andExpect(jsonPath("$.status").value("online"))
				.andExpect(jsonPath("$.storage").value("mysql"));
	}

	@Test
	void historyEndpointReturnsSeededData() throws Exception {
		mockMvc.perform(get("/api/history").param("hours", "24"))
				.andExpect(status().isOk())
				.andExpect(jsonPath("$.count").isNumber())
				.andExpect(jsonPath("$.records").isArray());
	}

	@Test
	void statsEndpointReturnsAverages() throws Exception {
		mockMvc.perform(get("/api/stats").param("hours", "24"))
				.andExpect(status().isOk())
				.andExpect(jsonPath("$.temperature.average").isNumber())
				.andExpect(jsonPath("$.humidity.average").isNumber());
	}

	@Test
	void telemetryEndpointRejectsMissingFields() throws Exception {
		mockMvc.perform(post("/api/telemetry")
				.contentType("application/json")
				.content("""
						{
						  "nodeId": 1,
						  "createdAt": "2026-03-19T21:00:00",
						  "temperature": 25.0,
						  "humidity": 60.0,
						  "soil": 35.0,
						  "source": "mqtt"
						}
						"""))
				.andExpect(status().isBadRequest())
				.andExpect(jsonPath("$.message").value("light is required"));
	}
}
