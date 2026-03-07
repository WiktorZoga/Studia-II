import { SkillBadge } from './SkillBadge';

interface SkillListProps {
    skills: string[];
}

export const SkillsList = ({ skills } : SkillListProps) => {
    return (
        <div>
            <h3>Skills</h3>
            <div style={{ display: 'flex', flexWrap: 'wrap', gap: '10px'}}>
                {skills.map((skill) => (
                    <SkillBadge key={skill} skillName={skill} />
                ))}
            </div>
        </div>
    );
};