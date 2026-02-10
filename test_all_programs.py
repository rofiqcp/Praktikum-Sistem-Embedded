#!/usr/bin/env python3
"""
Automated Build Test Script for All Programs
Tests all STM32 and ESP32 programs in the repository
"""

import os
import subprocess
import json
from datetime import datetime
from pathlib import Path
import sys

class ProgramTester:
    def __init__(self, base_dir):
        self.base_dir = Path(base_dir)
        self.results = {
            'total': 0,
            'passed': 0,
            'failed': 0,
            'skipped': 0,
            'failures': [],
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
    
    def test_project(self, project_path):
        """Test a single project by attempting to build it"""
        project_name = project_path.name
        module_name = project_path.parent.parent.parent.name
        platform = project_path.parent.name
        
        print(f"\n{'='*80}")
        print(f"Testing: {module_name}/{platform}/{project_name}")
        print(f"{'='*80}")
        
        try:
            # Run platformio check (compile without upload)
            result = subprocess.run(
                ['pio', 'run', '--project-dir', str(project_path)],
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout per project
            )
            
            if result.returncode == 0:
                print(f"✅ PASSED: {project_name}")
                return {
                    'status': 'passed',
                    'project': str(project_path.relative_to(self.base_dir)),
                    'module': module_name,
                    'platform': platform,
                    'name': project_name
                }
            else:
                print(f"❌ FAILED: {project_name}")
                print(f"Error output:\n{result.stderr[-500:]}")  # Last 500 chars
                return {
                    'status': 'failed',
                    'project': str(project_path.relative_to(self.base_dir)),
                    'module': module_name,
                    'platform': platform,
                    'name': project_name,
                    'error': result.stderr[-500:]
                }
                
        except subprocess.TimeoutExpired:
            print(f"⏱️ TIMEOUT: {project_name}")
            return {
                'status': 'failed',
                'project': str(project_path.relative_to(self.base_dir)),
                'module': module_name,
                'platform': platform,
                'name': project_name,
                'error': 'Build timeout (>5 minutes)'
            }
        except Exception as e:
            print(f"⚠️ ERROR: {project_name} - {str(e)}")
            return {
                'status': 'failed',
                'project': str(project_path.relative_to(self.base_dir)),
                'module': module_name,
                'platform': platform,
                'name': project_name,
                'error': str(e)
            }
    
    def run_all_tests(self):
        """Run tests on all projects"""
        print("🔍 Finding all PlatformIO projects...")
        projects = self.find_all_projects()
        
        print(f"\n📊 Found {len(projects)} projects to test")
        print("=" * 80)
        
        for i, project in enumerate(projects, 1):
            print(f"\n[{i}/{len(projects)}]", end=" ")
            result = self.test_project(project)
            
            self.results['total'] += 1
            if result['status'] == 'passed':
                self.results['passed'] += 1
            elif result['status'] == 'failed':
                self.results['failed'] += 1
                self.results['failures'].append(result)
            else:
                self.results['skipped'] += 1
        
        return self.results
    
    def generate_report(self, output_file='test_results.json'):
        """Generate a detailed test report"""
        # Save JSON report
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        # Generate markdown report
        md_report = self.generate_markdown_report()
        with open('TEST_REPORT.md', 'w') as f:
            f.write(md_report)
        
        # Print summary to console
        print("\n" + "=" * 80)
        print("📊 TEST SUMMARY")
        print("=" * 80)
        print(f"Total Projects:  {self.results['total']}")
        print(f"✅ Passed:       {self.results['passed']} ({self.results['passed']/self.results['total']*100:.1f}%)")
        print(f"❌ Failed:       {self.results['failed']} ({self.results['failed']/self.results['total']*100:.1f}%)")
        print(f"⏭️ Skipped:      {self.results['skipped']}")
        print("=" * 80)
        
        if self.results['failures']:
            print("\n❌ FAILED PROJECTS:")
            for failure in self.results['failures']:
                print(f"  - {failure['module']}/{failure['platform']}/{failure['name']}")
        
        print(f"\n📄 Detailed reports saved to:")
        print(f"  - test_results.json")
        print(f"  - TEST_REPORT.md")
    
    def generate_markdown_report(self):
        """Generate a markdown test report"""
        report = f"""# Test Report - All Programs

**Test Date:** {self.results['timestamp']}

## Summary

| Metric | Count | Percentage |
|--------|-------|------------|
| Total Projects | {self.results['total']} | 100% |
| ✅ Passed | {self.results['passed']} | {self.results['passed']/self.results['total']*100:.1f}% |
| ❌ Failed | {self.results['failed']} | {self.results['failed']/self.results['total']*100:.1f}% |
| ⏭️ Skipped | {self.results['skipped']} | {self.results['skipped']/self.results['total']*100:.1f}% |

## Results by Module

"""
        # Group results by module
        modules = {}
        for failure in self.results['failures']:
            module = failure['module']
            if module not in modules:
                modules[module] = {'passed': 0, 'failed': 0}
            modules[module]['failed'] += 1
        
        # Calculate passed by deduction
        all_projects = self.find_all_projects()
        for project in all_projects:
            module = project.parent.parent.parent.name
            if module not in modules:
                modules[module] = {'passed': 0, 'failed': 0}
        
        for module in sorted(modules.keys()):
            total_in_module = sum(1 for p in all_projects if module in str(p))
            passed = total_in_module - modules[module].get('failed', 0)
            failed = modules[module].get('failed', 0)
            report += f"### {module}\n"
            report += f"- ✅ Passed: {passed}/{total_in_module}\n"
            report += f"- ❌ Failed: {failed}/{total_in_module}\n\n"
        
        if self.results['failures']:
            report += "\n## Failed Projects\n\n"
            for failure in self.results['failures']:
                report += f"### {failure['module']}/{failure['platform']}/{failure['name']}\n"
                report += f"```\n{failure.get('error', 'Unknown error')}\n```\n\n"
        
        return report

def main():
    """Main entry point"""
    # Get the base directory (current directory)
    base_dir = os.getcwd()
    
    print("🚀 Starting Automated Build Tests")
    print(f"📁 Base directory: {base_dir}")
    
    # Create tester and run tests
    tester = ProgramTester(base_dir)
    results = tester.run_all_tests()
    
    # Generate reports
    tester.generate_report()
    
    # Exit with appropriate code
    if results['failed'] > 0:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()
