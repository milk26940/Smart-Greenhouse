package com.example.demo.dto;

import java.time.LocalDateTime;

public record PresenceResponse(
		String sensor1,
		String sensor2,
		String sensor3,
		String executor4,
		String executor5,
		LocalDateTime latestRecordAt
) {
}
