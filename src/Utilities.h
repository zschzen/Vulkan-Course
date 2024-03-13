#ifndef UTILITIES_H
#define UTILITIES_H

// Indices (locations) of Queue Families.
typedef struct queueFamilyIndices_t
{
	// Locations
	int graphicsFamily     = -1;
	int presentationFamily = -1;

	/** @brief Check if the queue families are valid */
	[[nodiscard]] inline bool IsValid() const { return graphicsFamily >= 0; }

} queueFamilyIndices_t;

#endif //UTILITIES_H
