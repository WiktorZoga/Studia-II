interface SkillBadgeProps {
    skillName: string;
}

export const SkillBadge = ({ skillName } : SkillBadgeProps) => {
    return (
        <span style={{
            border: '1px solid black',
            padding: '5px 10px',
            margin: '2px',
            borderRadius: '15px',
            backgroundColor: '#6a8a73ff',
        }}>
            {skillName}
        </span>
    );
};

