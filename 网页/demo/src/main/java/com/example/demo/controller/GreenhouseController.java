package com.example.demo.controller;

import com.example.demo.dto.HealthResponse;
import com.example.demo.dto.HistoryResponse;
import com.example.demo.dto.PresenceResponse;
import com.example.demo.dto.RealtimeResponse;
import com.example.demo.dto.StatsResponse;
import com.example.demo.dto.TelemetryUpsertRequest;
import com.example.demo.service.GreenhouseService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api")
public class GreenhouseController {

	private final GreenhouseService greenhouseService;

	public GreenhouseController(GreenhouseService greenhouseService) {
		this.greenhouseService = greenhouseService;
	}

	@GetMapping("/health")
	public HealthResponse health() {
		return greenhouseService.health();
	}

	@GetMapping("/realtime")
	public ResponseEntity<RealtimeResponse> realtime() {
		return greenhouseService.realtime()
				.map(ResponseEntity::ok)
				.orElseGet(() -> ResponseEntity.noContent().build());
	}

	@GetMapping("/history")
	public HistoryResponse history(@RequestParam(defaultValue = "24") int hours) {
		return greenhouseService.history(hours);
	}

	@GetMapping("/stats")
	public StatsResponse stats(@RequestParam(defaultValue = "24") int hours) {
		return greenhouseService.stats(hours);
	}

	@GetMapping("/presence")
	public PresenceResponse presence() {
		return greenhouseService.presence();
	}

	@PostMapping("/telemetry")
	public RealtimeResponse ingest(@RequestBody TelemetryUpsertRequest request) {
		return greenhouseService.ingest(request);
	}
}
