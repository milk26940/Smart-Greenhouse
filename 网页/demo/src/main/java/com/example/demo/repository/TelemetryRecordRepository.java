package com.example.demo.repository;

import com.example.demo.model.TelemetryRecord;
import org.springframework.data.jpa.repository.JpaRepository;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

public interface TelemetryRecordRepository extends JpaRepository<TelemetryRecord, Long> {

	Optional<TelemetryRecord> findTopByOrderByCreatedAtDesc();

	List<TelemetryRecord> findByCreatedAtBetweenOrderByCreatedAtAsc(LocalDateTime start, LocalDateTime end);
}
