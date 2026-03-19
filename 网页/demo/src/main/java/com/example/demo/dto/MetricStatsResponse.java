package com.example.demo.dto;

public record MetricStatsResponse(
		double average,
		double max,
		double min,
		double latest
) {
}
