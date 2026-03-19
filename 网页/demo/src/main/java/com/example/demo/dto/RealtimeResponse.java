package com.example.demo.dto;

import java.time.LocalDateTime;

public record RealtimeResponse(
		boolean hasData,
		String summary,
		String dataSource,
		LocalDateTime updatedAt,
		EnvPayload env,
		DecisionPayload decision,
		VisionPayload vision,
		PresencePayload presence
) {
	public record EnvPayload(
			double temperature,
			double humidity,
			double soil,
			int light,
			int co2
	) {
	}

	public record DecisionPayload(
			String fan,
			String pump,
			String led,
			String summary
	) {
	}

	public record VisionPayload(
			double brightness,
			double greenRatio,
			boolean motionFlag,
			boolean visionOk
	) {
	}

	public record PresencePayload(
			String sensor1,
			String sensor2,
			String sensor3,
			String executor4,
			String executor5
	) {
	}
}
