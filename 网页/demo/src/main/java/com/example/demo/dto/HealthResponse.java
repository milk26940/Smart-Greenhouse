package com.example.demo.dto;

public record HealthResponse(
		String status,
		String storage,
		String service
) {
}
