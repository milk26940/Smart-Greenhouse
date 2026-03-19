package com.example.demo.dto;

import java.time.LocalDateTime;

public record StatsResponse(
		LocalDateTime start,
		LocalDateTime end,
		long sampleCount,
		MetricStatsResponse temperature,
		MetricStatsResponse humidity,
		MetricStatsResponse soil,
		MetricStatsResponse light,
		MetricStatsResponse co2
) {
}
