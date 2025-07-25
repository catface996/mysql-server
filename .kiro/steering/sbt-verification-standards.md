---
inclusion: fileMatch
fileMatchPattern: 'storage/sbt/**'
---

# SBT Storage Engine - Task Verification Standards

## Overview

For the SBT storage engine project, every completed task must have a corresponding verification document in the `storage/sbt/verification/` directory. This ensures quality, traceability, and proper documentation of implementation progress.

## Verification Requirements

### Mandatory for Each Task
1. **Create verification document** before marking task as complete
2. **Document location**: `storage/sbt/verification/TASK_X_Y_VERIFICATION.md`
3. **Naming convention**: `TASK_[section]_[subsection]_VERIFICATION.md`
4. **Complete all sections** as specified in the template

### Verification Document Template

```markdown
# Task X.Y Implementation Verification

## Task Requirements
- [ ] Requirement 1
- [ ] Requirement 2
- [ ] ...

## Implementation Details

### 1. [Component Name] ✓
**Location**: `path/to/file.cc` (lines X-Y)

[Code snippet or description]

**Features**:
- Feature 1
- Feature 2

### 2. [Next Component] ✓
[Similar structure]

## Test Results

### [Test Category]
```
[Test output or results]
```

## Key Implementation Features
- ✅ Feature 1
- ✅ Feature 2

## Compliance with Requirements
- ✅ Requirement mapping
- ✅ Design compliance
- ✅ Code quality

## Conclusion
[Summary of completion status and achievements]
```

### Required Sections

1. **Task Requirements Checklist**
   - All requirements from tasks.md
   - Clear ✓/✗ status for each

2. **Implementation Details**
   - File locations with line numbers
   - Key code snippets
   - Architecture decisions

3. **Test Results**
   - Unit test outputs
   - Integration test results
   - Independent verification tests
   - Performance metrics (if applicable)

4. **Compliance Verification**
   - Requirements traceability
   - Design specification adherence
   - Code quality standards

5. **Conclusion**
   - Clear completion status
   - Key achievements
   - Readiness for next tasks

### Test Evidence Requirements

Each verification must include:
- **Unit test results** - All tests passing
- **Independent tests** - Standalone verification programs
- **Code coverage** - Key functionality tested
- **Error handling** - Edge cases covered
- **Performance** - Acceptable performance characteristics

### Quality Gates

Before marking a task complete:
- ✅ All requirements implemented
- ✅ All tests passing
- ✅ Code reviewed and documented
- ✅ No critical issues remaining
- ✅ Verification document complete
- ✅ Ready for dependent tasks

## File Organization

```
storage/sbt/verification/
├── README.md                    # Overview and guidelines
├── TASK_2_1_VERIFICATION.md     # Basic data structures
├── TASK_2_2_VERIFICATION.md     # Insert operations
├── TASK_2_3_VERIFICATION.md     # Delete operations
├── TASK_2_4_VERIFICATION.md     # Search and traversal
├── TASK_3_1_VERIFICATION.md     # File format
├── TASK_3_2_VERIFICATION.md     # Serialization
└── ...                          # Additional tasks
```

## Integration with Development Process

### When Starting a Task
1. Create verification document template
2. List all requirements to be implemented
3. Plan test strategy

### During Implementation
1. Update verification document with progress
2. Document key decisions and code locations
3. Add test results as they become available

### Before Completing Task
1. Ensure all requirements are met
2. Run all tests and document results
3. Complete verification document
4. Review for quality and completeness

### Task Completion Criteria
A task is only considered complete when:
- Implementation is finished
- All tests pass
- Verification document is complete
- Code is properly documented
- No critical issues remain

## Benefits

This verification process ensures:
- **Quality**: Every task is properly tested and documented
- **Traceability**: Clear mapping from requirements to implementation
- **Maintainability**: Future developers can understand implementation decisions
- **Reliability**: Comprehensive testing reduces bugs
- **Progress Tracking**: Clear visibility into project status

## Example Reference

See `storage/sbt/verification/TASK_2_1_VERIFICATION.md` for a complete example of proper verification documentation.

## Enforcement

- All task completions must include verification document
- Code reviews should verify documentation completeness
- CI/CD should validate test results match verification claims
- Project status reports should reference verification documents