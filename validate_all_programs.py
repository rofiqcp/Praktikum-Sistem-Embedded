#!/usr/bin/env python3
"""
Code Validation Script - Tests all programs without compilation
Validates structure, syntax, and configuration files
"""

import os
import re
from pathlib import Path
from datetime import datetime
import json

class ProgramValidator:
    def __init__(self, base_dir):
        self.base_dir = Path(base_dir)
        self.results = {
            'total': 0,
            'passed': 0,
            'failed': 0,
            'warnings': 0,
            'projects': [],
            'timestamp': datetime.now().isoformat()
        }
        
    def find_all_projects(self):
        """Find all PlatformIO projects in the repository"""
        projects = []
        for platformio_file in self.base_dir.rglob('platformio.ini'):
            project_dir = platformio_file.parent
            # Skip if it's in a build directory
            if '.pio' not in str(project_dir):
                projects.append(project_dir)
        return sorted(projects)
    
    def validate_platformio_ini(self, project_path):
        """Validate platformio.ini file"""
        issues = []
        ini_file = project_path / 'platformio.ini'
        
        try:
            with open(ini_file, 'r') as f:
                content = f.read()
                
            # Check for required sections
            if '[env:' not in content:
                issues.append("No environment configuration found")
            
            # Check for platform specification
            if 'platform =' not in content:
                issues.append("No platform specified")
                
            # Check for board specification
            if 'board =' not in content:
                issues.append("No board specified")
                
            # Check for framework
            if 'framework =' not in content:
                issues.append("No framework specified")
                
        except Exception as e:
            issues.append(f"Error reading platformio.ini: {str(e)}")
            
        return issues
    
    def validate_source_files(self, project_path):
        """Validate source code files"""
        issues = []
        src_dir = project_path / 'src'
        
        if not src_dir.exists():
            issues.append("No src/ directory found")
            return issues
        
        # Find source files
        source_files = list(src_dir.glob('*.c')) + list(src_dir.glob('*.cpp')) + list(src_dir.glob('*.ino'))
        
        if not source_files:
            issues.append("No source files found in src/")
            return issues
        
        # Check main file
        main_files = [f for f in source_files if 'main' in f.name.lower()]
        if not main_files:
            issues.append("No main source file found")
        
        # Basic syntax checks for each file
        for source_file in source_files:
            try:
                with open(source_file, 'r', errors='ignore') as f:
                    content = f.read()
                    
                # Check for basic C/C++ syntax markers
                if source_file.suffix in ['.c', '.cpp']:
                    # Check for unbalanced braces (simple check)
                    open_braces = content.count('{')
                    close_braces = content.count('}')
                    if abs(open_braces - close_braces) > 2:  # Allow some tolerance
                        issues.append(f"{source_file.name}: Possibly unbalanced braces")
                    
                    # Check if file is empty or too small
                    if len(content.strip()) < 10:
                        issues.append(f"{source_file.name}: File appears empty or too small")
                        
            except Exception as e:
                issues.append(f"{source_file.name}: Error reading file - {str(e)}")
        
        return issues
    
    def validate_project_structure(self, project_path):
        """Validate project directory structure"""
        issues = []
        
        # Check for required directories/files
        if not (project_path / 'src').exists():
            issues.append("Missing src/ directory")
        
        if not (project_path / 'platformio.ini').exists():
            issues.append("Missing platformio.ini file")
        
        # Check if include directory exists (optional but common)
        if not (project_path / 'include').exists():
            # This is just a warning, not critical
            pass
        
        return issues
    
    def validate_project(self, project_path):
        """Validate a single project"""
        project_name = project_path.name
        module_name = project_path.parent.parent.parent.name
        platform = project_path.parent.name
        
        print(f"\n{'='*80}")
        print(f"Validating: {module_name}/{platform}/{project_name}")
        print(f"{'='*80}")
        
        issues = []
        warnings = []
        
        # Validate project structure
        structure_issues = self.validate_project_structure(project_path)
        issues.extend(structure_issues)
        
        # Validate platformio.ini
        ini_issues = self.validate_platformio_ini(project_path)
        issues.extend(ini_issues)
        
        # Validate source files
        source_issues = self.validate_source_files(project_path)
        issues.extend(source_issues)
        
        # Determine status
        status = 'passed'
        if len(issues) > 0:
            # Critical issues
            critical = any('Missing' in issue or 'No' in issue and 'found' in issue for issue in issues)
            if critical:
                status = 'failed'
                print(f"❌ FAILED: {len(issues)} critical issues")
            else:
                status = 'warning'
                warnings = issues
                issues = []
                print(f"⚠️  WARNING: {len(warnings)} minor issues")
        else:
            print(f"✅ PASSED")
        
        result = {
            'status': status,
            'project': str(project_path.relative_to(self.base_dir)),
            'module': module_name,
            'platform': platform,
            'name': project_name,
            'issues': issues,
            'warnings': warnings
        }
        
        # Print issues
        for issue in issues:
            print(f"   ❌ {issue}")
        for warning in warnings:
            print(f"   ⚠️  {warning}")
        
        return result
    
    def run_all_validations(self):
        """Run validation on all projects"""
        print("🔍 Finding all PlatformIO projects...")
        projects = self.find_all_projects()
        
        print(f"\n📊 Found {len(projects)} projects to validate")
        print("=" * 80)
        
        for i, project in enumerate(projects, 1):
            print(f"\n[{i}/{len(projects)}]", end=" ")
            result = self.validate_project(project)
            
            self.results['total'] += 1
            self.results['projects'].append(result)
            
            if result['status'] == 'passed':
                self.results['passed'] += 1
            elif result['status'] == 'failed':
                self.results['failed'] += 1
            elif result['status'] == 'warning':
                self.results['warnings'] += 1
                self.results['passed'] += 1  # Count as passed but with warnings
        
        return self.results
    
    def generate_report(self):
        """Generate validation reports"""
        # Save JSON report
        with open('validation_results.json', 'w') as f:
            json.dump(self.results, f, indent=2)
        
        # Generate markdown report
        md_report = self.generate_markdown_report()
        with open('VALIDATION_REPORT.md', 'w') as f:
            f.write(md_report)
        
        # Print summary to console
        print("\n" + "=" * 80)
        print("📊 VALIDATION SUMMARY")
        print("=" * 80)
        print(f"Total Projects:  {self.results['total']}")
        print(f"✅ Passed:       {self.results['passed']} ({self.results['passed']/self.results['total']*100:.1f}%)")
        print(f"❌ Failed:       {self.results['failed']} ({self.results['failed']/self.results['total']*100:.1f}%)")
        print(f"⚠️  Warnings:     {self.results['warnings']}")
        print("=" * 80)
        
        # Show failed projects
        failed_projects = [p for p in self.results['projects'] if p['status'] == 'failed']
        if failed_projects:
            print("\n❌ FAILED PROJECTS:")
            for project in failed_projects:
                print(f"  - {project['module']}/{project['platform']}/{project['name']}")
                for issue in project['issues']:
                    print(f"      • {issue}")
        
        print(f"\n📄 Detailed reports saved to:")
        print(f"  - validation_results.json")
        print(f"  - VALIDATION_REPORT.md")
    
    def generate_markdown_report(self):
        """Generate a markdown validation report"""
        report = f"""# Program Validation Report

**Validation Date:** {self.results['timestamp']}

## Summary

| Metric | Count | Percentage |
|--------|-------|------------|
| Total Projects | {self.results['total']} | 100% |
| ✅ Passed | {self.results['passed']} | {self.results['passed']/self.results['total']*100:.1f}% |
| ❌ Failed | {self.results['failed']} | {self.results['failed']/self.results['total']*100:.1f}% |
| ⚠️ Warnings | {self.results['warnings']} | {self.results['warnings']/self.results['total']*100:.1f}% |

## Validation Checks Performed

1. ✅ Project structure validation
   - Presence of src/ directory
   - Presence of platformio.ini
   - Optional include/ directory

2. ✅ PlatformIO configuration validation
   - Environment configuration
   - Platform specification
   - Board specification
   - Framework specification

3. ✅ Source code validation
   - Presence of source files
   - Main file existence
   - Basic syntax checks
   - Brace balance checking

## Results by Module

"""
        # Group results by module
        modules = {}
        for project in self.results['projects']:
            module = project['module']
            if module not in modules:
                modules[module] = {'passed': 0, 'failed': 0, 'warnings': 0, 'total': 0}
            modules[module]['total'] += 1
            if project['status'] == 'failed':
                modules[module]['failed'] += 1
            elif project['status'] == 'warning':
                modules[module]['warnings'] += 1
            else:
                modules[module]['passed'] += 1
        
        for module in sorted(modules.keys()):
            stats = modules[module]
            report += f"### {module}\n"
            report += f"- Total: {stats['total']}\n"
            report += f"- ✅ Passed: {stats['passed']}\n"
            report += f"- ❌ Failed: {stats['failed']}\n"
            report += f"- ⚠️ Warnings: {stats['warnings']}\n\n"
        
        # Failed projects detail
        failed_projects = [p for p in self.results['projects'] if p['status'] == 'failed']
        if failed_projects:
            report += "\n## Failed Projects\n\n"
            for project in failed_projects:
                report += f"### ❌ {project['module']}/{project['platform']}/{project['name']}\n\n"
                if project['issues']:
                    report += "**Issues:**\n"
                    for issue in project['issues']:
                        report += f"- {issue}\n"
                report += "\n"
        
        # Projects with warnings
        warning_projects = [p for p in self.results['projects'] if p['status'] == 'warning']
        if warning_projects:
            report += "\n## Projects with Warnings\n\n"
            for project in warning_projects[:20]:  # Show first 20
                report += f"### ⚠️ {project['module']}/{project['platform']}/{project['name']}\n\n"
                if project['warnings']:
                    report += "**Warnings:**\n"
                    for warning in project['warnings']:
                        report += f"- {warning}\n"
                report += "\n"
            if len(warning_projects) > 20:
                report += f"\n*... and {len(warning_projects) - 20} more projects with warnings*\n"
        
        return report

def main():
    """Main entry point"""
    base_dir = os.getcwd()
    
    print("🚀 Starting Program Validation")
    print(f"📁 Base directory: {base_dir}")
    print("\nNote: This validates project structure and configuration")
    print("      Full compilation testing requires PlatformIO platforms to be installed")
    
    validator = ProgramValidator(base_dir)
    results = validator.run_all_validations()
    validator.generate_report()
    
    # Exit code based on failures
    return 0 if results['failed'] == 0 else 1

if __name__ == '__main__':
    exit(main())
